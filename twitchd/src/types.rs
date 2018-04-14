extern crate hyper;
extern crate hyper_tls;

use hyper::{Client, client::HttpConnector};
use hyper_tls::HttpsConnector;

pub type HttpsClient = Client<HttpsConnector<HttpConnector>>;
