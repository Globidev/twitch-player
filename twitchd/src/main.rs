#[macro_use]
extern crate serde_derive;
#[macro_use]
extern crate nom;

extern crate hyper;
extern crate hyper_tls;
extern crate tokio_core;

extern crate num_cpus;

extern crate futures;

mod twitch;
mod state;
mod types;

use futures::{Future, Stream, future};

use std::rc::Rc;
use std::cell::RefCell;

struct TwitchdApi {
    index_cache: Rc<RefCell<state::index_cache::IndexCache>>
}

impl hyper::server::Service for TwitchdApi {
    type Request = hyper::server::Request;
    type Response = hyper::server::Response;
    type Error = hyper::Error;
    type Future = Box<Future<Item = Self::Response, Error = Self::Error>>;

    fn call(&self, req: Self::Request) -> Self::Future {
        let channel = &req.path()[1..];
        println!("{}", channel);

        let mut cache = self.index_cache.borrow_mut();
        let future = cache.get(channel)
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

fn main() {
    use ::hyper_tls::HttpsConnector;

    let mut core = tokio_core::reactor::Core::new().unwrap();
    let handle = &core.handle();

    let client = hyper::Client::configure()
        .connector(HttpsConnector::new(num_cpus::get(), handle).unwrap())
        .build(handle);

    let index_cache = state::index_cache::IndexCache::new(client);
    let index_cache_rc = Rc::new(RefCell::new(index_cache));

    let addr = "0.0.0.0:8080".parse().unwrap();
    let serve = hyper::server::Http::new()
        .serve_addr_handle(&addr, handle, ||
            Ok(TwitchdApi { index_cache: Rc::clone(&index_cache_rc) })
        ).unwrap();

    let server = serve.for_each(|incoming| {
        let future = incoming.map_err(|e| println!("Unexpected error: {}", e));
        handle.spawn(future);
        futures::future::ok(())
    });

    core.run(server).unwrap();
}
