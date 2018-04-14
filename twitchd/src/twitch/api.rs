extern crate futures;
extern crate hyper;
extern crate num_cpus;
extern crate serde_json;
extern crate tokio_core;
extern crate url;

use types::HttpsClient;
use super::types::{AccessToken, StreamIndex};

use self::futures::{Future, Stream, future};

type ApiFuture<T> = Box<Future<Item = T, Error = ApiError>>;

fn fetch(client: &HttpsClient, request: hyper::Request)
    -> impl Future<Item = hyper::Chunk, Error = ApiError>
{
    use hyper::StatusCode;

    client.request(request)
        .map_err(ApiError::from)
        .and_then(move |response| -> ApiFuture<_> {
            match response.status() {
                StatusCode::Ok => {
                    let read_body = response
                        .body()
                        .concat2()
                        .map_err(ApiError::from);
                    Box::new(read_body)
                },
                status => Box::new(future::err(ApiError::BadStatus(status)))
            }
        })
}

fn access_token_uri(channel: &str) -> hyper::Uri {
    let url = format!(
        "http://api.twitch.tv/api/channels/{}/access_token",
        channel.to_lowercase()
    );
    url.parse().unwrap()
}

pub fn access_token(client: &HttpsClient, channel: &str, client_id: &str)
    -> impl Future<Item = AccessToken, Error = ApiError>
{
    use self::hyper::{Request, Get};
    use self::serde_json::from_slice as json_decode;

    let mut request = Request::new(Get, access_token_uri(channel));
    request.headers_mut()
        .set_raw("Client-ID", client_id);

    fetch(client, request)
        .and_then(|chunk| -> ApiFuture<_> {
            match json_decode(&chunk) {
                Ok(token)  => Box::new(future::ok(token)),
                Err(error) => {
                    let api_error = ApiError::FormatError(error.to_string());
                    Box::new(future::err(api_error))
                }
            }
        })
}

fn stream_index_uri(channel: &str, token: &AccessToken) -> hyper::Uri {
    use self::url::form_urlencoded;

    let query_string = form_urlencoded::Serializer::new(String::new())
        .append_pair("token",        &token.token)
        .append_pair("sig",          &token.signature)
        .append_pair("allow_source", "true")
        .append_pair("type",         "any")
        .finish();

    let url = format!(
        "http://usher.twitch.tv/api/channel/hls/{}.m3u8?{}",
        channel.to_lowercase(),
        query_string
    );

    url.parse().unwrap()
}

pub fn stream_index(client: &HttpsClient, channel: &str, token: &AccessToken)
    -> impl Future<Item = StreamIndex, Error = ApiError>
{
    use self::hyper::{Request, Get};
    use super::m3u8::{ParseError, parse_stream_index};

    let request = Request::new(Get, stream_index_uri(channel, token));

    fetch(client, request)
        .and_then(|chunk| -> ApiFuture<_> {
            match parse_stream_index(&chunk) {
                Ok(index)  => Box::new(future::ok(index)),
                Err(ParseError(error)) => {
                    let api_error = ApiError::FormatError(error);
                    Box::new(future::err(api_error))
                }
            }
        })
}

use std::{error, fmt};

#[derive(Debug)]
pub enum ApiError {
    NetworkError(hyper::Error),
    BadStatus(hyper::StatusCode),
    FormatError(String)
}

impl error::Error for ApiError {
    fn description(&self) -> &str { "Api Error" }
}

impl fmt::Display for ApiError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl From<hyper::Error> for ApiError {
    fn from(error: hyper::Error) -> Self {
        ApiError::NetworkError(error)
    }
}
