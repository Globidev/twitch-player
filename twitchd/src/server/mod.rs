extern crate hyper;

mod service;
mod state;

use self::service::TwitchdApi;
use self::state::State;

use prelude::futures::*;
use options::Options;

use std::net::SocketAddr;
use std::sync::Arc;

pub fn run(opts: Options) {
    let addr = SocketAddr::new(opts.host, opts.port);

    let (shutdown_sender, sutdown_receiver) = sync::oneshot::channel();
    let state = State::new(opts, shutdown_sender);
    let shared_state = Arc::new(state);

    let make_service = move || {
        Ok(TwitchdApi::new(&shared_state)) as hyper::Result<TwitchdApi>
    };

    let server = hyper::Server::bind(&addr)
        .serve(make_service)
        .map_err(|e| eprintln!("server error: {}", e));

    let server_with_shutdown_signal = server
        .select(sutdown_receiver.then(|_| Ok(())))
        .then(|_| Ok(()));

    hyper::rt::run(server_with_shutdown_signal);
}
