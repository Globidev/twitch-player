extern crate hyper;
extern crate futures;
extern crate url;
extern crate serde;
extern crate serde_json;

use self::hyper::server;
use self::futures::{Future, future};

use super::state::{index_cache::IndexError, State};

use std::rc::Rc;
use std::collections::HashMap;

use twitch::types::StreamIndex;

pub struct TwitchdApi {
    state: Rc<State>
}

type ApiRequest = server::Request;
type ApiResponse = server::Response;
type ApiError = hyper::Error;
type ApiFuture = Box<Future<Item = ApiResponse, Error = ApiError>>;

type QueryParams = HashMap<String, String>;

impl TwitchdApi {
    pub fn new(state: &Rc<State>) -> Self {
        Self { state: Rc::clone(state) }
    }

    fn get_stream_index(&self, channel: &str) -> ApiFuture {
        let return_error = |error| {
            let response = match error {
                IndexError::NotFound          => not_found(),
                IndexError::Unexpected(error) => server_error(&error)
            };
            Ok(response)
        };

        let response = self.state.index_cache.get(channel)
            .map(json_response)
            .or_else(return_error);

        Box::new(response)
    }

    fn play_stream(&self, channel: &str, params: &QueryParams) -> ApiFuture {
        let stream = (String::from(channel), params.get("quality").map(Clone::clone).unwrap_or_default());
        if self.state.player_pool.is_playing(&stream) {
            let (sink, body) = hyper::Body::pair();
            self.state.player_pool.add_client(stream, sink);
            let response = server::Response::new()
                .with_body(body);
            respond(response)
        } else {
            let return_error = |error| {
                let response = match error {
                    IndexError::NotFound          => not_found(),
                    IndexError::Unexpected(error) => server_error(&error)
                };
                Ok(response)
            };

            let stream_response = {
                let state = Rc::clone(&self.state);
                move |index: StreamIndex| {
                    let (sink, body) = hyper::Body::pair();
                    let playlist_info = &index.playlist_infos[0];
                    state.player_pool.add_player(stream, playlist_info.clone(), sink);
                    let response = server::Response::new()
                        .with_body(body);
                    response
                }
            };

            let response = self.state.index_cache.get(channel)
                .map(stream_response)
                .or_else(return_error);

            Box::new(response)
        }
    }
}

impl server::Service for TwitchdApi {
    type Request = ApiRequest;
    type Response = ApiResponse;
    type Error = ApiError;
    type Future = ApiFuture;

    fn call(&self, req: Self::Request) -> Self::Future {
        use self::hyper::Method::Get;
        use self::url::form_urlencoded::parse as parse_query;

        let query_bytes = req.query().unwrap_or_default().as_bytes();
        let params: QueryParams = parse_query(query_bytes)
            .map(|(k, v)| (String::from(k), String::from(v)))
            .collect();

        match (req.method(), req.path()) {
            // Stream index
            (Get, "/stream_index") => {
                match params.get("channel") {
                    None          => respond(bad_request("Missing channel")),
                    Some(channel) => self.get_stream_index(&channel)
                }
            },
            // Video stream
            (Get, "/play") => {
                match params.get("channel") {
                    None          => respond(bad_request("Missing channel")),
                    Some(channel) => self.play_stream(&channel, &params)
                }
            },
            // Default => 404
            _ => respond(not_found())
        }
    }
}

fn not_found() -> server::Response {
    server::Response::new()
        .with_status(hyper::StatusCode::NotFound)
}

fn bad_request(detail: &str) -> server::Response {
    server::Response::new()
        .with_status(hyper::StatusCode::BadRequest)
        .with_body(String::from(detail))
}

fn server_error(detail: &str) -> server::Response {
    server::Response::new()
        .with_status(hyper::StatusCode::InternalServerError)
        .with_body(String::from(detail))
}

fn json_response(value: impl serde::Serialize) -> server::Response {
    use self::serde_json::to_vec as encode;
    use self::hyper::{header, mime};

    let reply_with_data = |data: Vec<u8>| {
        server::Response::new()
            .with_header(header::ContentLength(data.len() as u64))
            .with_header(header::ContentType(mime::APPLICATION_JSON))
            .with_body(data)
    };

    let reply_with_error = |error| {
        let detail = format!("Encoding error: {}", error);
        server::Response::new()
            .with_status(hyper::StatusCode::InternalServerError)
            .with_body(detail)
    };

    encode(&value)
        .map(reply_with_data)
        .unwrap_or_else(reply_with_error)
}

fn respond(response: server::Response) -> ApiFuture {
    Box::new(future::ok(response))
}
