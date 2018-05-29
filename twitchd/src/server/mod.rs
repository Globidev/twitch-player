extern crate futures;
extern crate hyper;
extern crate tokio_core;

mod service;
mod state;

use prelude::asio::Core;
use prelude::futures::*;
use options::Options;

use std::net::SocketAddr;
use std::rc::Rc;

pub fn run(opts: Options) {
    let mut core = Core::new().unwrap();
    let handle = &core.handle();

    let addr = SocketAddr::new(opts.host, opts.port);
    let (shutdown_signal_sender, shutdown_signal_receiver) = sync::oneshot::channel();
    let state = Rc::new(state::State::new(opts, shutdown_signal_sender, handle));
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

    let server_with_shutdown_signal = server
        .select(shutdown_signal_receiver.then(|_| Ok(())));

    if let Err(_) = core.run(server_with_shutdown_signal) {
        println!("Failed to run server");
    }
}
