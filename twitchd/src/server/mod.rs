mod service;
mod state;

use self::service::TwitchdApi;
use self::state::State;

use crate::prelude::futures::*;
use crate::options::Options;

use std::net::SocketAddr;
use std::sync::Arc;
use hyper::service::{make_service_fn, service_fn};

use futures::channel::oneshot;

pub fn run(opts: Options) {
    let mut runtime = opts.runtime_strategy.init_runtime();

    let addr = SocketAddr::new(opts.host, opts.port);

    let (shutdown_sender, shutdown_receiver) = oneshot::channel();
    let state = State::new(opts, shutdown_sender);
    let shared_state = Arc::new(state);

    let make_service = make_service_fn(move |_| {
        let shared_state = shared_state.clone();
        async move {
            Ok::<_, hyper::Error>(service_fn(move |req| {
                let shared_state = shared_state.clone();
                async move {
                    let mut api = TwitchdApi::new(&shared_state);
                    api.call(req).await
                }
            }))
        }
    });

    runtime.block_on(async {
        let server = hyper::Server::bind(&addr)
            .serve(make_service)
            .map_err(|e| eprintln!("server error: {}", e));

        let server_with_shutdown_signal = future::select(
            server,
            shutdown_receiver
        ).map(|_| Ok::<_, ()>(()));

        server_with_shutdown_signal.await
    }).expect("Failed to run server");
}
