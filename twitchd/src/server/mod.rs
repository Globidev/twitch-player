mod service;
mod state;

use service::TwitchdApi;
use state::State;

use crate::options::Options;

use std::net::SocketAddr;
use std::sync::Arc;
use std::convert::Infallible;

use hyper::service::{make_service_fn, service_fn};
use thiserror::Error;

use futures::{prelude::*, channel::oneshot};

pub fn run(opts: Options) {
    let mut runtime = opts.runtime_strategy.init_runtime();

    let addr = SocketAddr::new(opts.host, opts.port);

    let (shutdown_sender, shutdown_receiver) = oneshot::channel();
    let state = State::new(opts, shutdown_sender);
    let shared_state = Arc::new(state);

    let make_service = make_service_fn(move |_| {
        let shared_state = shared_state.clone();
        async move {
            Ok::<_, Infallible>(service_fn(move |req| {
                let shared_state = shared_state.clone();
                async move {
                    let mut api = TwitchdApi::new(&shared_state);
                    Ok::<_, Infallible>(api.call(req).await)
                }
            }))
        }
    });

    runtime
        .block_on(async {
            let server = hyper::Server::bind(&addr)
                .serve(make_service);

            let server_with_shutdown = future::select(
                server.map_err(ServerError::Hyper),
                shutdown_receiver.map_err(ServerError::ShutdownCanceled)
            );

            let (serve_result, _) = server_with_shutdown.await.factor_first();
            serve_result
        })
        .expect("Failed to run server");
}

#[derive(Debug, Error)]
enum ServerError {
    #[error("Lost shutdown capability somwhow")]
    ShutdownCanceled(oneshot::Canceled),
    #[error("Server error: {0}")]
    Hyper(hyper::Error)
}
