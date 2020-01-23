use crate::prelude::http::*;
use crate::prelude::runtime;
use crate::options::Options;
use crate::twitch::api::{ApiError, access_token, stream_index};
use crate::twitch::types::{StreamIndex, Channel};

use std::sync::Arc;
use futures::lock::Mutex;
use futures::prelude::*;
use std::collections::HashMap;

use tokio::time::delay_for;

type FutureStreamIndex = future::BoxFuture<'static, Arc<Result<StreamIndex, IndexError>>>;
type ChannelIndexes = HashMap<Channel, future::Shared<FutureStreamIndex>>;

pub struct IndexCache {
    opts: Options,
    client: HttpsClient,
    cache: Arc<Mutex<ChannelIndexes>>,
}

impl IndexCache {
    pub fn new(opts: Options) -> Self {
        Self {
            opts,
            client: http_client(),
            cache: Default::default(),
        }
    }

    pub async fn get(&self, channel: &Channel, oauth: Option<impl AsRef<str>>)
        -> Result<StreamIndex, IndexError>
    {
        let opt_index = self.cache.lock().await.get(&channel).cloned();

        match opt_index {
            Some(shared_index) => (*shared_index.await).clone(),
            None => {
                let shared_index = self.create_new_shared_index(
                    channel.clone(),
                    oauth.as_ref().map(AsRef::as_ref)
                );

                shared_index.await
            }
        }
    }

    async fn create_new_shared_index(&self, channel: Channel, oauth: Option<&str>)
        -> Result<StreamIndex, IndexError>
    {
        let client = self.client.clone();
        let client_id = self.opts.client_id.to_string();
        let oauth = oauth.map(String::from);

        let future_index = {
            let channel = channel.clone();
            async move {
                let token = access_token(&client, channel.name(), &client_id, oauth.as_deref()).await?;

                stream_index(&client, channel.name(), &token).err_into().await
            }
        };

        let shared_index = future_index
            .map(Arc::new)
            .boxed()
            .shared();

        self.cache.lock().await.insert(channel.clone(), shared_index.clone());

        let index = (*shared_index.await).clone();

        let expire_cache = {
            let cache = self.cache.clone();
            async move {
                cache.lock().await.remove(&channel);
            }
        };

        match index {
            Err(_) => expire_cache.await,
            Ok(_) => {
                let expire_later = delay_for(self.opts.index_cache_timeout)
                    .then(|_| expire_cache);

                runtime::spawn(expire_later);
            }
        }

        index
    }
}

#[derive(thiserror::Error, Debug, Clone)]
pub enum IndexError {
    #[error("Not found")]
    NotFound,
    #[error("Unexpected: {0}")]
    Unexpected(String)
}

impl From<ApiError> for IndexError {
    fn from(error: ApiError) -> Self {
        use HttpError::BadStatus;
        use hyper::StatusCode;

        match error {
            ApiError::Http(BadStatus(StatusCode::NOT_FOUND)) => IndexError::NotFound,
            _ => IndexError::Unexpected(error.to_string())
        }
    }
}
