extern crate futures;

use futures::{Future, future};

use std::collections::HashMap;

use types::HttpsClient;

use twitch::api::ApiError;
use twitch::types::StreamIndex;

type Channel = String;
type ChannelIndexes = HashMap<Channel, StreamIndex>;

const CLIENT_ID: &str = "alcht5gave31yctuzv48v2i1hlwq53";

use std::rc::Rc;
use std::cell::RefCell;

pub struct IndexCache {
    client: Rc<HttpsClient>,
    cache: Rc<RefCell<ChannelIndexes>>
}

impl IndexCache {
    pub fn new(client: HttpsClient) -> Self {
        Self {
            client: Rc::new(client),
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

        let cache_result = {
            let channel = channel.clone();
            let cache = Rc::clone(&self.cache);
            move |index: StreamIndex| {
                cache.borrow_mut().insert(channel, index.clone());
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
        use hyper::StatusCode::NotFound;

        match error {
            ApiError::BadStatus(NotFound) => IndexError::NotFound,
            _ => IndexError::Unexpected(error.to_string())
        }
    }
}
