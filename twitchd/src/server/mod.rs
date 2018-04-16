extern crate futures;
extern crate hyper;
extern crate tokio_core;

mod service;
mod state;

use prelude::asio::Core;
use prelude::futures::*;

use std::net::SocketAddr;
use std::rc::Rc;

pub fn run(addr: &SocketAddr) {
    let mut core = Core::new().unwrap();
    let handle = &core.handle();

    let state = Rc::new(state::State::new(handle));
    let make_service = || Ok(service::TwitchdApi::new(&state));

    let incoming_stream = hyper::server::Http::new()
        .serve_addr_handle(&addr, handle, make_service)
        .unwrap();

    let server = incoming_stream.for_each(|incoming| {
        let client_future = incoming
            .map_err(|e| eprintln!("Error handling client: {}", e));
        handle.spawn(client_future);
        Ok(())
    });

    core.run(server).unwrap();
}
