extern crate hyper;
extern crate hyper_tls;
extern crate tokio_core;

use hyper::{Client, client::HttpConnector};
use hyper_tls::HttpsConnector;

pub type HttpsClient = Client<HttpsConnector<HttpConnector>>;
pub type Handle = tokio_core::reactor::Handle;
