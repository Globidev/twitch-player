use crate::prelude::http::*;
use crate::prelude::futures::*;
use crate::prelude::runtime;
use crate::options::Options;
use crate::twitch::api::ApiError;
use crate::twitch::types::StreamIndex;

use std::sync::Arc;
use futures::lock::Mutex;
use std::collections::HashMap;

use tokio::time::delay_for;

type Channel = String;
type FutureStreamIndex = future::BoxFuture<'static, Arc<Result<StreamIndex, IndexError>>>;
type ChannelIndexes = HashMap<Channel, future::Shared<FutureStreamIndex>>;
type Cache = Arc<Mutex<ChannelIndexes>>;

pub struct IndexCache {
    opts: Options,
    client: HttpsClient,
    cache: Cache,
}

impl IndexCache {
    pub fn new(opts: Options) -> Self {
        Self {
            opts,
            client: http_client(),
            cache: Default::default(),
        }
    }

    pub async fn get(&self, channel: &str, oauth: Option<&str>)
        -> Result<StreamIndex, IndexError>
    {
        let channel_lc = channel.to_lowercase();

        let opt_index = self.cache.lock().await.get(&channel_lc)
            .cloned();

        match opt_index {
            Some(shared_index) => (*shared_index.await).clone(),
            None => {
                let shared_index = self.create_new_shared_index(
                    &channel_lc,
                    oauth
                );

                shared_index.await
            }
        }
    }

    async fn create_new_shared_index(&self, channel: &str, oauth: Option<&str>)
        -> Result<StreamIndex, IndexError>
    {
        let channel = channel.to_string();
        let client = self.client.clone();
        let client_id = self.opts.client_id.to_string();
        let oauth = oauth.map(String::from);

        let future_index = fetch_stream_index(
            client,
            channel.clone(),
            client_id,
            oauth
        );

        let shared_index = future_index
            .map(Arc::new)
            .boxed()
            .shared();

        self.cache.lock().await.insert(channel.clone(), shared_index.clone());

        let index = (*shared_index.await).clone();

        match index {
            Err(_) => {
                self.cache.lock().await.remove(&channel);
            },
            Ok(_) => {
                let timeout = self.opts.index_cache_timeout;
                let cache = self.cache.clone();
                runtime::spawn(async move {
                    delay_for(timeout).await;
                    cache.lock().await.remove(&channel);
                });
            }
        }

        index
    }
}

async fn fetch_stream_index(client: HttpsClient, channel: String, client_id: String, oauth: Option<String>)
    -> Result<StreamIndex, IndexError>
{
    use crate::twitch::api::{access_token, stream_index};

    let token = access_token(&client, &channel, &client_id, oauth.as_deref()).await?;

    stream_index(&client, &channel, &token).err_into().await
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
