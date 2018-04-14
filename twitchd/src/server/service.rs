extern crate hyper;
extern crate futures;

use self::hyper::server;
use self::futures::{Future, future};

use super::state::State;

use std::rc::Rc;

pub struct TwitchdApi {
    state: Rc<State>
}

impl TwitchdApi {
    pub fn new(state: &Rc<State>) -> Self {
        Self { state: Rc::clone(state) }
    }
}

impl server::Service for TwitchdApi {
    type Request = server::Request;
    type Response = server::Response;
    type Error = hyper::Error;
    type Future = Box<Future<Item = Self::Response, Error = Self::Error>>;

    fn call(&self, req: Self::Request) -> Self::Future {
        let channel = &req.path()[1..];
        println!("{}", channel);

        let future = self.state.index_cache.get(channel)
            .map(|index| {
                hyper::server::Response::new()
                    .with_body(format!("{:#?}", index))
            })
            .or_else(|error| {
                future::ok(hyper::server::Response::new()
                    .with_status(hyper::StatusCode::NotFound))
            });

        Box::new(future)
    }
}
