use crate::prelude::http::*;
use crate::twitch::types::{Quality, Stream};
use crate::twitch::utils::find_playlist;

use super::state::{State, index_cache::IndexError};

const DAEMON_VERSION: &str = "1.4.8";

pub struct TwitchdApi<'st> {
    state: &'st State
}

impl TwitchdApi<'_> {
    pub fn new(state: &State) -> TwitchdApi<'_> {
        TwitchdApi { state }
    }

    pub async fn call(&self, req: Request) -> Response {
        let params = {
            let query_params = req.uri().query()
                .map(parse_query_params)
                .unwrap_or_default();

            ApiParams(query_params)
        };

        let result = match (req.method(), req.uri().path()) {
            // Capabilities
            (&Method::GET, "/stream_index")  => self.get_stream_index(params).await,
            (&Method::GET, "/play")          => self.get_video_stream(params).await,
            (&Method::GET, "/meta")          => self.get_metadata(params).await,
            // Utilities
            (&Method::GET,  "/version")      => Ok(Self::get_version()),
            (&Method::POST, "/quit")         => Ok(self.post_quit().await),
            // Default => 404
            _ => Err(ApiError::NotFound)
        };

        result.into_response()
    }

    async fn get_stream_index(&self, params: ApiParams) -> ApiResponse {
        let channel = params.get("channel")?;
        let oauth = params.get_opt("oauth");

        let index = self.state.index_cache.get(channel, oauth).await?;

        Ok(json_response(index))
    }

    async fn get_video_stream(&self, params: ApiParams) -> ApiResponse {
        let channel = params.get("channel")?;

        let quality = params.get_opt("quality")
            .map(Quality::from)
            .unwrap_or_default();

        let meta_key = params.get_opt("meta_key")
            .map_or_else(Default::default, ToOwned::to_owned);

        let stream = (channel.to_string(), quality.clone());

        let (sink, response) = streaming_response();

        self.state.player_pool.entry(stream)
            .or_try_insert_with(|| async {
                let oauth = params.get_opt("oauth");
                let index = self.state.index_cache.get(channel, oauth).await?;
                find_playlist(index, &quality).ok_or(ApiError::NotFound)
            })
            .play(sink, meta_key)
            .await?;

        Ok(response)
    }

    async fn get_metadata(&self, params: ApiParams) -> ApiResponse {
        let channel = params.get("channel")?;
        let key = params.get("key")?;

        let quality = params.get("quality")
            .map(Quality::from)
            .unwrap_or_default();

        let stream = (channel.to_string(), quality);

        let metadata = self.state.player_pool.get_metadata(&stream, &key.to_owned())
            .await
            .ok_or(ApiError::NotFound)?;

        Ok(json_response(metadata))
    }

    fn get_version() -> Response {
        Response::new(DAEMON_VERSION.into())
    }

    async fn post_quit(&self) -> Response {
        let mut signal_guard = self.state.shutdown_signal.lock().await;

        if let Some(signal) = signal_guard.take() {
            signal.send(()).unwrap_or_default();
        }

        Response::default()
    }
}

struct ApiParams(QueryParams);

impl ApiParams {
    fn get(&self, key: &str) -> Result<&str, ApiError> {
        self.get_opt(key)
            .ok_or_else(|| ApiError::BadRequest(format!("Missing {}", key)))
    }

    fn get_opt(&self, key: &str) -> Option<&str> {
        self.0.get(key).map(String::as_str)
    }
}

#[derive(Debug)]
enum ApiError {
    NotFound,
    Index(IndexError),
    BadRequest(String),
}

impl From<IndexError> for ApiError {
    fn from(err: IndexError) -> Self { Self::Index(err) }
}

trait IntoResponse {
    fn into_response(self) -> Response;
}

impl IntoResponse for Result<Response, ApiError> {
    fn into_response(self) -> Response {
        self.unwrap_or_else(|err| match err {
            ApiError::NotFound => not_found(),
            ApiError::Index(err) => index_error_response(err),
            ApiError::BadRequest(detail) => bad_request(&detail),
        })
    }
}

type ApiResponse = Result<Response, ApiError>;

fn not_found() -> Response {
    hyper::Response::builder()
        .status(hyper::StatusCode::NOT_FOUND)
        .body(Default::default())
        .expect("Response building error: Not Found")
}

fn bad_request(detail: &str) -> Response {
    hyper::Response::builder()
        .status(hyper::StatusCode::BAD_REQUEST)
        .body(Vec::from(detail).into())
        .expect("Response building error: Bad request")
}

fn server_error(detail: &str) -> Response {
    hyper::Response::builder()
        .status(hyper::StatusCode::INTERNAL_SERVER_ERROR)
        .body(Vec::from(detail).into())
        .expect("Response building error: Server Error")
}

fn json_response(value: impl serde::Serialize) -> Response {
    use serde_json::to_vec as encode;

    let reply_with_data = |data: Vec<u8>| {
        hyper::Response::builder()
            .header(header::CONTENT_LENGTH, data.len().to_string().as_str())
            .header(header::CONTENT_TYPE, mime::APPLICATION_JSON.as_ref())
            .body(data.into())
            .expect("Response building error: Json Data")
    };

    let reply_with_error = |error| {
        let detail = format!("Encoding error: {}", error);
        server_error(&detail)
    };

    encode(&value)
        .map(reply_with_data)
        .unwrap_or_else(reply_with_error)
}

fn index_error_response(error: IndexError) -> Response {
    match error {
        IndexError::NotFound => not_found(),
        IndexError::Unexpected(error) => server_error(&error)
    }
}
