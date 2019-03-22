use crate::prelude::futures::*;
use crate::prelude::http::*;
use crate::twitch::types::{StreamIndex, Quality, Stream};
use crate::twitch::utils::find_playlist;

use super::state::{State, index_cache::IndexError};

use std::sync::Arc;

const DAEMON_VERSION: &str = "1.3.1";

type ApiRequest = Request;
type ApiResponse = Response;
type ApiBody = hyper::Body;
type ApiError = hyper::Error;
pub type ApiFuture = BoxFuture<ApiResponse, ApiError>;

pub struct TwitchdApi {
    state: Arc<State>
}

impl TwitchdApi {
    pub fn new(state: &Arc<State>) -> Self {
        Self { state: Arc::clone(state) }
    }

    fn get_stream_index(&self, params: QueryParams) -> ApiFuture {
        match params.get("channel") {
            None          => respond(bad_request("Missing channel")),
            Some(channel) => {
                let response = self.state.index_cache.get(channel)
                    .map(json_response)
                    .or_else(|error| Ok(index_error_response(error)));

                Box::new(response)
            }
        }
    }

    fn get_video_stream(&self, params: QueryParams) -> ApiFuture {
        match params.get("channel") {
            None          => respond(bad_request("Missing channel")),
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
                    respond(response)
                } else {
                    self.fetch_and_play(stream, meta_key)
                }
            }
        }
    }

    fn get_metadata(&self, params: QueryParams) -> ApiFuture {
        match (params.get("channel"), params.get("key")) {
            (None, _)                  => respond(bad_request("Missing channel")),
            (_, None)                  => respond(bad_request("Missing key")),
            (Some(channel), Some(key)) => {
                let quality = params.get("quality")
                    .map(|raw_quality| Quality::from(raw_quality.clone()))
                    .unwrap_or_default();
                let stream = (channel.clone(), quality);

                match self.state.player_pool.get_metadata(&stream, key) {
                    Some(metadata) => respond(json_response(metadata)),
                    None        => respond(not_found())
                }
            }
        }
    }

    fn get_version(&self) -> ApiFuture {
        let response = ApiResponse::new(ApiBody::from(DAEMON_VERSION));
        respond(response)
    }

    fn quit(&self) -> ApiFuture {
        if let Some(signal) = (*self.state.shutdown_signal.lock().unwrap()).take() {
            signal.send(()).unwrap_or_default();
        }
        let response = ApiResponse::default();
        respond(response)
    }

    fn fetch_and_play(&self, stream: Stream, meta_key: Option<String>) -> ApiFuture {
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

        let response = self.state.index_cache.get(&channel)
            .map(stream_response)
            .or_else(|error| Ok(index_error_response(error)));

        Box::new(response)
    }
}

impl hyper::service::Service for TwitchdApi {
    type ReqBody = ApiBody;
    type ResBody = ApiBody;
    type Error = ApiError;
    type Future = ApiFuture;

    fn call(&mut self, req: ApiRequest) -> Self::Future {
        let params = req.uri().query()
            .map(parse_query_params)
            .unwrap_or_default();

        match (req.method(), req.uri().path()) {
            // Capabilities
            (&Method::GET, "/stream_index")  => self.get_stream_index(params),
            (&Method::GET, "/play")          => self.get_video_stream(params),
            (&Method::GET, "/meta")          => self.get_metadata(params),
            // Utilities
            (&Method::GET,  "/version")      => self.get_version(),
            (&Method::POST, "/quit")         => self.quit(),
            // Default => 404
            _ => respond(not_found())
        }
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

fn respond(response: ApiResponse) -> ApiFuture {
    Box::new(future::ok(response))
}
