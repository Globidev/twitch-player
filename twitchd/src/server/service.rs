extern crate hyper;
extern crate futures;
extern crate url;
extern crate serde_json;

use self::hyper::server;
use self::futures::{Future, future};

use super::state::{index_cache::IndexError, State};

use std::rc::Rc;
use std::collections::HashMap;

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
        use self::serde_json::to_string as encode;

        let return_index = |index| {
            use self::hyper::mime::APPLICATION_JSON;

            let return_encode_error = |error| {
                let detail = format!("Encoding error: {}", error);
                server::Response::new()
                    .with_status(hyper::StatusCode::InternalServerError)
                    .with_body(detail)
            };

            encode(&index)
                .map(|data|
                    server::Response::new()
                        .with_header(hyper::header::ContentLength(data.len() as u64))
                        .with_header(hyper::header::ContentType(APPLICATION_JSON))
                        .with_body(data)
                )
                .unwrap_or_else(return_encode_error)
        };

        let return_error = |error| {
            match error {
                IndexError::NotFound          => not_found(),
                IndexError::Unexpected(error) => server_error(&error)
            }
        };

        let response = self.state.index_cache.get(channel)
            .map(return_index)
            .or_else(return_error);

        Box::new(response)
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
                    None          => bad_request("Missing channel"),
                    Some(channel) => self.get_stream_index(&channel)
                }
            },
            // Default => 404
            _ => not_found()
        }
    }
}

fn not_found() -> ApiFuture {
    use self::hyper::StatusCode::NotFound;

    let response = server::Response::new()
        .with_status(NotFound);
    Box::new(future::ok(response))
}

fn bad_request(detail: &str) -> ApiFuture {
    use self::hyper::StatusCode::BadRequest;

    let response = server::Response::new()
        .with_status(BadRequest)
        .with_body(String::from(detail));
    Box::new(future::ok(response))
}

fn server_error(detail: &str) -> ApiFuture {
    use self::hyper::StatusCode::InternalServerError;

    let response = server::Response::new()
        .with_status(InternalServerError)
        .with_body(String::from(detail));
    Box::new(future::ok(response))
}
