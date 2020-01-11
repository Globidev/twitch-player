use crate::prelude::futures::*;
use crate::prelude::http::*;
use crate::twitch::types::{StreamIndex, Quality, Stream};
use crate::twitch::utils::find_playlist;

use super::state::{State, index_cache::IndexError};

use std::sync::Arc;

const DAEMON_VERSION: &str = "1.4.8";

type ApiBody = hyper::Body;
type ApiRequest = hyper::Request<ApiBody>;
type ApiResponse = hyper::Response<ApiBody>;
type ResponseResult = Result<ApiResponse, hyper::Error>;

// type ApiResponse = Response;
// type ApiBody = hyper::Body;
// type ApiError = hyper::Error;
// pub type ApiFuture = BoxFuture<ApiResponse, ApiError>;

pub struct TwitchdApi {
    state: Arc<State>
}

impl TwitchdApi {
    pub fn new(state: &Arc<State>) -> Self {
        Self { state: Arc::clone(state) }
    }

    pub async fn call(&mut self, req: ApiRequest) -> ResponseResult {
        let params = req.uri().query()
            .map(parse_query_params)
            .unwrap_or_default();

        match (req.method(), req.uri().path()) {
            // Capabilities
            (&Method::GET, "/stream_index")  => self.get_stream_index(params).await,
            (&Method::GET, "/play")          => self.get_video_stream(params).await,
            (&Method::GET, "/meta")          => self.get_metadata(params).await,
            // Utilities
            (&Method::GET,  "/version")      => Ok(self.get_version()),
            (&Method::POST, "/quit")         => self.quit().await,
            // Default => 404
            _ => Ok(not_found())
        }
    }

    async fn get_stream_index(&self, params: QueryParams) -> ResponseResult {
        match params.get("channel") {
            None => Ok(bad_request("Missing channel")),
            Some(channel) => {
                let oauth = params.get("oauth").map(String::as_str);

                let response = self.state.index_cache.get(channel, oauth).await
                    .map(json_response)
                    .unwrap_or_else(index_error_response);

                Ok(response)
            }
        }
    }

    async fn get_video_stream(&self, params: QueryParams) -> ResponseResult {
        match params.get("channel") {
            None => Ok(bad_request("Missing channel")),
            Some(channel) => {
                let quality = params.get("quality")
                    .map(|raw_quality| Quality::from(raw_quality.clone()))
                    .unwrap_or_default();
                let meta_key = params.get("meta_key")
                    .cloned();
                let stream = (channel.clone(), quality);

                if self.state.player_pool.is_playing(&stream) {
                    let (sink, response) = streaming_response();
                    self.state.player_pool.add_sink(&stream, sink, meta_key);
                    Ok(response)
                } else {
                    let oauth = params.get("oauth").map(String::as_str);
                    self.fetch_and_play(stream, meta_key, oauth).await
                }
            }
        }
    }

    async fn get_metadata(&self, params: QueryParams) -> ResponseResult {
        match (params.get("channel"), params.get("key")) {
            (None, _) => Ok(bad_request("Missing channel")),
            (_, None) => Ok(bad_request("Missing key")),
            (Some(channel), Some(key)) => {
                let quality = params.get("quality")
                    .map(|raw_quality| Quality::from(raw_quality.clone()))
                    .unwrap_or_default();
                let stream = (channel.clone(), quality);

                match self.state.player_pool.get_metadata(&stream, key) {
                    Some(metadata) => Ok(json_response(metadata)),
                    None => Ok(not_found())
                }
            }
        }
    }

    fn get_version(&self) -> Response {
        Response::new(hyper::Body::from(DAEMON_VERSION))
    }

    async fn quit(&self) -> ResponseResult {
        if let Some(signal) = (*self.state.shutdown_signal.lock().unwrap()).take() {
            signal.send(()).unwrap_or_default();
        }
        Ok(Response::default())
    }

    async fn fetch_and_play(&self, stream: Stream, meta_key: Option<String>, oauth: Option<&str>)
        -> ResponseResult
    {
        let (channel, quality) = stream.clone();

        let stream_response = {
            let state = Arc::clone(&self.state);
            move |index: StreamIndex| {
                match find_playlist(index, &quality) {
                    None           => not_found(),
                    Some(playlist) => {
                        let (sink, response) = streaming_response();
                        state.player_pool
                            .add_player(stream, playlist, sink, meta_key);
                        response
                    }
                }
            }
        };

        let response = self.state.index_cache.get(&channel, oauth)
            .await
            .map(stream_response)
            .unwrap_or_else(index_error_response);

        Ok(response)
    }
}

fn not_found() -> ApiResponse {
    hyper::Response::builder()
        .status(hyper::StatusCode::NOT_FOUND)
        .body(ApiBody::default())
        .expect("Response building error: Not Found")
}

fn bad_request(detail: &str) -> ApiResponse {
    hyper::Response::builder()
        .status(hyper::StatusCode::BAD_REQUEST)
        .body(ApiBody::from(Vec::from(detail)))
        .expect("Response building error: Bad request")
}

fn server_error(detail: &str) -> ApiResponse {
    hyper::Response::builder()
        .status(hyper::StatusCode::INTERNAL_SERVER_ERROR)
        .body(ApiBody::from(Vec::from(detail)))
        .expect("Response building error: Server Error")
}

fn json_response(value: impl serde::Serialize) -> ApiResponse {
    use serde_json::to_vec as encode;

    let reply_with_data = |data: Vec<u8>| {
        hyper::Response::builder()
            .header(header::CONTENT_LENGTH, data.len().to_string().as_str())
            .header(header::CONTENT_TYPE, mime::APPLICATION_JSON.as_ref())
            .body(ApiBody::from(data))
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

fn index_error_response(error: IndexError) -> ApiResponse {
    match error {
        IndexError::NotFound          => not_found(),
        IndexError::Unexpected(error) => server_error(&error)
    }
}
