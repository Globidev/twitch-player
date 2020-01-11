use super::futures::*;

use std::collections::HashMap;

pub use http::{header, method::Method};
use hyper::{body::Bytes, client::HttpConnector};
use hyper_tls::HttpsConnector;

pub type Request = hyper::Request<hyper::Body>;
pub type Response = hyper::Response<hyper::Body>;
pub type HttpsClient = hyper::Client<HttpsConnector<HttpConnector>>;
pub type QueryParams = HashMap<String, String>;
pub type ResponseSink = hyper::body::Sender;

pub fn http_client() -> HttpsClient {
    let connector = HttpsConnector::new();

    hyper::Client::builder().build(connector)
}

pub fn parse_query_params(query: &str) -> QueryParams {
    use url::form_urlencoded::parse as parse_query;

    parse_query(query.as_bytes())
        .map(|(k, v)| (String::from(k), String::from(v)))
        .collect()
}

pub async fn fetch(
    client: &HttpsClient,
    request: Request,
) -> Result<Vec<u8>, HttpError> {
    let mut chunks = fetch_streamed(client, request);
    let mut concat = Vec::new();

    while let Some(chunk) = chunks.next().await {
        concat.extend(chunk?);
    }

    Ok(concat)
}

pub fn fetch_streamed(
    client: &HttpsClient,
    request: Request,
) -> impl Stream<Item = Result<Bytes, HttpError>> + Unpin {
    client
        .request(request)
        .map_err(HttpError::NetworkError)
        .and_then(|resp| future::ready(stream_response(resp)))
        .try_flatten_stream()
}

pub fn streaming_response() -> (ResponseSink, Response) {
    let (sink, body) = hyper::Body::channel();
    let response = Response::new(body);

    (sink, response)
}

fn stream_response(
    response: Response,
) -> Result<impl Stream<Item = Result<Bytes, HttpError>> + Unpin, HttpError> {
    match response.status() {
        hyper::StatusCode::OK => {
            Ok(response.into_body().map_err(HttpError::NetworkError))
        }
        status => Err(HttpError::BadStatus(status)),
    }
}

#[derive(Debug)]
pub enum HttpError {
    NetworkError(hyper::Error),
    BadStatus(hyper::StatusCode),
}

use std::{error, fmt};

impl error::Error for HttpError {
    fn description(&self) -> &str {
        "Http Error"
    }
}

impl fmt::Display for HttpError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}
