extern crate hyper;
extern crate tokio;

mod service;
mod state;

use self::service::TwitchdApi;
use self::state::State;

use prelude::futures::*;
use options::Options;

use std::net::SocketAddr;
use std::sync::Arc;

pub fn run(opts: Options) {
    let mut runtime = opts.runtime_strategy.init_runtime();

    let addr = SocketAddr::new(opts.host, opts.port);

    let (shutdown_sender, shutdown_receiver) = sync::oneshot::channel();
    let state = State::new(opts, shutdown_sender);
    let shared_state = Arc::new(state);

    let make_service = move || {
        Ok::<_, hyper::Error>(TwitchdApi::new(&shared_state))
    };

    let server = hyper::Server::bind(&addr)
        .serve(make_service)
        .map_err(|e| eprintln!("server error: {}", e));

    let server_with_shutdown_signal = server
        .select2(shutdown_receiver)
        .then(|_| Ok::<_, ()>(()));

    runtime.block_on(server_with_shutdown_signal)
        .expect("Failed to run server");
}
