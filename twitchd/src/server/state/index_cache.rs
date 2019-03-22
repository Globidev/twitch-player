use crate::prelude::http::*;
use crate::prelude::futures::*;
use crate::prelude::runtime;
use crate::options::Options;
use crate::twitch::api::ApiError;
use crate::twitch::types::StreamIndex;

use std::sync::{Arc, Mutex};
use std::collections::HashMap;
use std::time::{Instant, Duration};

use tokio::timer::Delay;

type Channel = String;
type FutureStreamIndex = BoxFuture<StreamIndex, IndexError>;
type ChannelIndexes = HashMap<Channel, future::Shared<FutureStreamIndex>>;
type Cache = Arc<Mutex<ChannelIndexes>>;

// Bug with type aliases used in associated types of impl traits return types
#[allow(dead_code)]
type SharedIndex = future::SharedItem<StreamIndex>;
#[allow(dead_code)]
type SharedError = future::SharedError<IndexError>;

pub struct IndexCache {
    opts: Options,
    client: HttpsClient,
    cache: Cache,
}

impl IndexCache {
    pub fn new(opts: Options) -> Self {
        Self {
            opts: opts,
            client: http_client().expect("Failed to initialize HTTP client"),
            cache: Default::default(),
        }
    }

    pub fn get(&self, channel: &str, oauth: Option<&str>)
        -> impl Future<Item = StreamIndex, Error = IndexError>
    {
        let channel_lc = channel.to_lowercase();

        let mut cache = self.cache.lock().unwrap();

        let opt_index = cache.get(&channel_lc)
            .cloned();

        let future_shared_index: BoxFuture<_, _> = match opt_index {
            Some(shared_index) => Box::new(shared_index),
            None => {
                let shared_index = self.create_new_shared_index(
                    &mut cache,
                    &channel_lc,
                    oauth
                );

                Box::new(shared_index)
            }
        };

        future_shared_index
            .map(|shared_item| (*shared_item).clone())
            .map_err(|shared_err| (*shared_err).clone())
    }

    fn create_new_shared_index(&self, cache: &mut ChannelIndexes, channel: &str, oauth: Option<&str>)
        -> impl Future<Item = SharedIndex, Error = SharedError>
    {
        let future_index = fetch_stream_index(
            &self.client,
            channel,
            &self.opts.client_id,
            oauth
        );

        let shared_index = (Box::new(future_index) as BoxFuture<_, _>)
            .shared();

        let schedule_cache_expiry = cache_expiry_scheduler(
            Arc::clone(&self.cache),
            channel,
            self.opts.index_cache_timeout
        );

        cache.insert(String::from(channel), shared_index.clone());

        shared_index.then(schedule_cache_expiry)
    }
}

fn fetch_stream_index(client: &HttpsClient, channel: &str, client_id: &str, oauth: Option<&str>)
    -> impl Future<Item = StreamIndex, Error = IndexError>
{
    use crate::twitch::api::{access_token, stream_index};

    let fetch_token = access_token(client, &channel, client_id, oauth);

    let fetch_index = {
        let client = client.clone();
        let channel = String::from(channel);
        move |token| stream_index(&client, &channel, &token)
    };

    fetch_token
        .and_then(fetch_index)
        .map_err(IndexError::from)
}

fn cache_expiry_scheduler<T, E>(cache: Cache, channel: &str, timeout: Duration)
    -> impl FnOnce(Result<T, E>) -> Result<T, E>
{
    let expire_cache = {
        let channel = String::from(channel);
        move || {
            cache.lock().unwrap().remove(&channel);
        }
    };

    move |result| {
        match result {
            Err(_) => expire_cache(),
            Ok(_)  => {
                let expire_at = Instant::now() + timeout;
                let expire_cache_later = Delay::new(expire_at)
                    .then(move |_| {
                        expire_cache();
                        Ok(())
                    });

                runtime::spawn(expire_cache_later);
            }
        }
        result
    }
}

#[derive(Clone)]
pub enum IndexError {
    NotFound,
    Unexpected(String)
}

impl From<ApiError> for IndexError {
    fn from(error: ApiError) -> Self {
        use self::ApiError::*;
        use self::HttpError::BadStatus;
        use hyper::StatusCode;

        match error {
            HttpError(BadStatus(StatusCode::NOT_FOUND)) => IndexError::NotFound,
            _ => IndexError::Unexpected(error.to_string())
        }
    }
}
