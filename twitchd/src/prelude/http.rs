use futures::prelude::*;

use std::{borrow::Cow, collections::HashMap};

pub use http::{header, method::Method};
use hyper::{body::Bytes, client::HttpConnector};
use hyper_tls::HttpsConnector;

pub type Request = hyper::Request<hyper::Body>;
pub type Response = hyper::Response<hyper::Body>;
pub type HttpsClient = hyper::Client<HttpsConnector<HttpConnector>>;
pub type QueryParams<'a> = HashMap<Cow<'a, str>, Cow<'a, str>>;
pub type ResponseSink = hyper::body::Sender;

pub fn http_client() -> HttpsClient {
    let connector = HttpsConnector::new();

    hyper::Client::builder().build(connector)
}

pub fn parse_query_params(query: &str) -> QueryParams<'_> {
    use url::form_urlencoded::parse as parse_query;

    parse_query(query.as_bytes())
        .collect()
}

pub fn fetch(
    client: &HttpsClient,
    request: Request,
) -> impl Future<Output = Result<Vec<u8>, HttpError>> {
    let mut chunks = fetch_streamed(client, request);

    async move {
        let mut concat = Vec::new();

        while let Some(chunk) = chunks.next().await {
            concat.extend(chunk?);
        }

        Ok(concat)
    }
}

pub fn fetch_streamed(
    client: &HttpsClient,
    request: Request,
) -> impl Stream<Item = Result<Bytes, HttpError>> + Unpin {
    client
        .request(request)
        .map(|response_result| {
            let response = response_result?;
            match response.status() {
                hyper::StatusCode::OK => Ok(response.into_body().err_into()),
                status => Err(HttpError::BadStatus(status)),
            }
        })
        .try_flatten_stream()
}

pub fn streaming_response() -> (ResponseSink, Response) {
    let (sink, body) = hyper::Body::channel();
    let response = Response::new(body);

    (sink, response)
}

#[derive(thiserror::Error, Debug)]
pub enum HttpError {
    #[error("Network error: {0}")]
    NetworkError(#[from] hyper::Error),
    #[error("Unexpected HTTP status: {0}")]
    BadStatus(hyper::StatusCode),
}
