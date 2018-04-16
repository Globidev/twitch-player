extern crate hyper;
extern crate tokio_timer;

use prelude::http::{HttpsClient, HttpError, http_client};
use prelude::asio::Handle;
use prelude::futures::*;

use twitch::api::ApiError;
use twitch::types::StreamIndex;

use self::tokio_timer::Timer;

use std::rc::Rc;
use std::cell::RefCell;
use std::time::Duration;
use std::collections::HashMap;

type Channel = String;
type ChannelIndexes = HashMap<Channel, StreamIndex>;

const CLIENT_ID: &str = "alcht5gave31yctuzv48v2i1hlwq53";
const EXPIRE_TIMEOUT: Duration = Duration::from_secs(60);

pub struct IndexCache {
    client: Rc<HttpsClient>,
    handle: Rc<Handle>,
    cache: Rc<RefCell<ChannelIndexes>>
}

impl IndexCache {
    pub fn new(handle: &Handle) -> Self {
        Self {
            client: Rc::new(http_client(handle).unwrap()),
            handle: Rc::new(handle.clone()),
            cache: Rc::new(RefCell::new(HashMap::new()))
        }
    }

    pub fn get(&self, channel: &str)
        -> Box<Future<Item = StreamIndex, Error = IndexError>>
    {
        let channel_lc = channel.to_lowercase();
        let cached_index = self.cache.borrow()
            .get(&channel_lc)
            .map(Clone::clone);

        match cached_index {
            Some(index) => Box::new(future::ok(index)),
            None        => Box::new(self.fetch_and_cache(channel_lc))
        }
    }

    fn fetch_and_cache(&self, channel: String)
        -> impl Future<Item = StreamIndex, Error = IndexError>
    {
        use twitch::api::{access_token, stream_index};

        let fetch_token = access_token(&self.client, &channel, CLIENT_ID);

        let fetch_index = {
            let client = Rc::clone(&self.client);
            let channel = channel.clone();
            move |token| stream_index(&client, &channel, &token)
        };

        let expire_cache = {
            let channel = channel.clone();
            let cache = Rc::clone(&self.cache);
            move |_| {
                cache.borrow_mut().remove(&channel);
                Ok(())
            }
        };

        let expire_cache_later = Timer::default()
            .sleep(EXPIRE_TIMEOUT)
            .then(expire_cache);

        let cache_result = {
            let channel = channel.clone();
            let cache = Rc::clone(&self.cache);
            let handle = Rc::clone(&self.handle);
            move |index: StreamIndex| {
                cache.borrow_mut().insert(channel, index.clone());
                handle.spawn(expire_cache_later);
                Ok(index)
            }
        };

        fetch_token
            .and_then(fetch_index)
            .and_then(cache_result)
            .map_err(IndexError::from)
    }
}

pub enum IndexError {
    NotFound,
    Unexpected(String)
}

impl From<ApiError> for IndexError {
    fn from(error: ApiError) -> Self {
        use self::hyper::StatusCode::NotFound;
        use self::HttpError::BadStatus;

        match error {
            ApiError::HttpError(BadStatus(NotFound)) => IndexError::NotFound,
            _ => IndexError::Unexpected(error.to_string())
        }
    }
}
