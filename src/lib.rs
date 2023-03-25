#![allow(clippy::missing_safety_doc)]

use tokio::{net, runtime, sync::oneshot};

pub struct Aio {
    rt_handle: runtime::Handle,
    rt_thread: std::thread::JoinHandle<Result<(), oneshot::error::RecvError>>,
    shutdown_tx: oneshot::Sender<()>,
}

#[no_mangle]
pub extern "C" fn aio__construct() -> *const Aio {
    println!("aio__construct");

    let rt = match runtime::Builder::new_current_thread()
        .enable_io()
        .enable_time()
        .build()
    {
        Ok(rt) => rt,
        Err(e) => {
            eprintln!("error constructing aio instance: {e}");
            return std::ptr::null();
        }
    };
    let rt_handle = rt.handle().clone();

    let (shutdown_tx, shutdown_rx) = oneshot::channel();
    let rt_thread = std::thread::spawn(move || {
        rt.block_on(async { shutdown_rx.await })?;
        rt.shutdown_timeout(std::time::Duration::from_millis(500));
        println!("Runtime shutdown");
        Ok(())
    });

    Box::into_raw(Box::new(Aio {
        rt_handle,
        rt_thread,
        shutdown_tx,
    }))
}

#[no_mangle]
pub unsafe extern "C" fn aio__destruct(ptr: *mut Aio) {
    if !ptr.is_null() {
        let aio = Box::from_raw(ptr);
        drop(aio.rt_handle);
        if aio.shutdown_tx.send(()).is_err() {
            eprintln!("error sending shutdown mesg");
        }
        if let Err(e) = aio.rt_thread.join() {
            eprintln!("error joining rt thread: {e:?}");
        }
        println!("aio__destruct");
    }
}

// Note: Rust can't track the lifetime of raw pointers and thus won't let us pass
// it across threads here (not Send), so we cheat and receive it as a usize.
#[no_mangle]
pub unsafe extern "C" fn aio__sleep(
    ptr: *const Aio,
    ms: u32,
    ctx: usize,
    cb: Option<extern "C" fn(usize)>,
) {
    if !ptr.is_null() {
        if let Some(cb) = cb {
            let aio = &*ptr;
            aio.rt_handle.spawn(async move {
                tokio::time::sleep(std::time::Duration::from_millis(u64::from(ms))).await;
                cb(ctx);
            });
        }
    }
}

// TODO: expose MissedTickBehavior?
#[no_mangle]
pub unsafe extern "C" fn aio__interval(
    ptr: *const Aio,
    ms: u32,
    ctx: usize,
    cb: Option<extern "C" fn(usize) -> u8>,
) {
    if !ptr.is_null() {
        if let Some(cb) = cb {
            let aio = &*ptr;
            aio.rt_handle.spawn(async move {
                let mut interval =
                    tokio::time::interval(std::time::Duration::from_millis(u64::from(ms)));
                loop {
                    interval.tick().await;
                    if cb(ctx) == 0 {
                        break;
                    }
                }
            });
        }
    }
}

pub struct UdpSocket {
    rt_handle: runtime::Handle,
    sock: std::sync::Arc<net::UdpSocket>,
    rx_handle: Option<tokio::task::JoinHandle<()>>,
}

#[no_mangle]
pub unsafe extern "C" fn aio__udp_bind(ptr: *const Aio, ip: u32, port: u16) -> *const UdpSocket {
    if !ptr.is_null() {
        let aio = &*ptr;
        let (tx, rx) = oneshot::channel();

        aio.rt_handle.spawn(async move {
            tx.send(net::UdpSocket::bind((std::net::Ipv4Addr::from(ip), port)).await)
        });

        if let Ok(Ok(sock)) = rx.blocking_recv() {
            println!("UDP socket bound to {}", sock.local_addr().unwrap());
            Box::into_raw(Box::new(UdpSocket {
                rt_handle: aio.rt_handle.clone(),
                sock: std::sync::Arc::new(sock),
                rx_handle: None,
            }))
        } else {
            std::ptr::null()
        }
    } else {
        std::ptr::null()
    }
}

#[no_mangle]
pub unsafe extern "C" fn aio__udp_destruct(ptr: *mut UdpSocket) {
    if !ptr.is_null() {
        let socket = Box::from_raw(ptr);
        println!(
            "destructing UDP socket at {}",
            socket.sock.local_addr().unwrap()
        );
        drop(socket);
    }
}

#[no_mangle]
pub unsafe extern "C" fn aio__udp_set_broadcast(ptr: *const UdpSocket, on: u8) -> u8 {
    if !ptr.is_null() {
        let socket = &*ptr;
        if socket.sock.set_broadcast(on != 0).is_ok() {
            1
        } else {
            0
        }
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn aio__udp_recv_from(
    ptr: *mut UdpSocket,
    ctx: usize,
    max_size: u32,
    cb: Option<extern "C" fn(ctx: usize, buf: *mut u8, len: u32, ip: u32, port: u16)>,
) {
    if !ptr.is_null() {
        let socket = &mut *ptr;

        if let Some(rx_handle) = socket.rx_handle.take() {
            rx_handle.abort();
        }

        if let Some(cb) = cb {
            socket.rx_handle = Some(socket.rt_handle.spawn({
                let sock = socket.sock.clone();
                async move {
                    let mut buf = vec![0u8; usize::try_from(max_size).unwrap()];
                    loop {
                        match sock.recv_from(&mut buf).await {
                            Ok((len, std::net::SocketAddr::V4(addr))) => {
                                cb(
                                    ctx,
                                    buf.as_mut_ptr(),
                                    u32::try_from(len).unwrap_or(0),
                                    u32::from(*addr.ip()),
                                    addr.port(),
                                );
                            }
                            Ok((_, std::net::SocketAddr::V6(_))) => {}
                            Err(e) => {
                                eprintln!(
                                    "udp recv_from error for {}: {e}",
                                    sock.local_addr().unwrap()
                                );
                                break;
                            }
                        }
                    }
                }
            }));
        }
    }
}

// Note: this function blocks since the caller is unable to determine the lifetime
// of the buffer and can't concurrently modify it.
#[no_mangle]
pub unsafe extern "C" fn aio__udp_send_to(
    ptr: *const UdpSocket,
    buf: *const u8,
    len: u32,
    ip: u32,
    port: u16,
) -> u32 {
    if !ptr.is_null() && !buf.is_null() {
        let socket = &*ptr;
        let buf = std::slice::from_raw_parts(buf, usize::try_from(len).unwrap());

        let (tx, rx) = oneshot::channel();
        socket.rt_handle.spawn(async move {
            tx.send(
                socket
                    .sock
                    .send_to(buf, (std::net::Ipv4Addr::from(ip), port))
                    .await,
            )
        });

        match rx.blocking_recv() {
            Ok(Ok(len)) => u32::try_from(len).unwrap_or(0),
            Ok(Err(e)) => {
                eprintln!("send_to error: {e}");
                0
            }
            Err(e) => {
                eprintln!("send_to rx error: {e}");
                0
            }
        }
    } else {
        0
    }
}
