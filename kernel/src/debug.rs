struct SimpleLogger;
struct UartLogger;

use log::{Record, Level, Metadata, LevelFilter, error};
use core::panic::PanicInfo;
use core::fmt::Write;
use spin::Mutex;


#[path = "arch/x86_64/mod.rs"]
pub mod arch;

static LOGGER: SimpleLogger = SimpleLogger;
static ULOGGER: Mutex<UartLogger> = Mutex::new(UartLogger);

impl Write for UartLogger {
    fn write_str(self: &mut UartLogger, data: &str) -> Result<(), core::fmt::Error> {
        for byte in data.bytes() {
            unsafe { arch::outb(0xE9, byte as u8); }
        }

        return Ok(())
    }
}

impl log::Log for SimpleLogger {
    fn enabled(&self, metadata: &Metadata) -> bool {
        metadata.level() <= Level::Trace
    }

    fn log(&self, record: &Record) {
        if self.enabled(record.metadata()) {
            let _ = write!(ULOGGER.lock(), "{}", record.args());
        }
    }
    
    // FLushing is not really required
    fn flush(&self) {}
}

pub fn init() {
    let _ = log::set_logger(&LOGGER).map(|()| log::set_max_level(LevelFilter::Info));
}

struct BackTrace {
    nxt: u64,
    ip: u64
}

impl BackTrace {
    fn decode(max_frames: u32) {
        let mut stk_ptr: *mut BackTrace;
        unsafe {
            asm!(
                "mov {0}, rbp",
                out(reg) stk_ptr
            );
        }

        for _i in 0..max_frames {
            let cf = unsafe { &*stk_ptr};
            error!("  -   {:?}\n", cf.ip);
            stk_ptr = cf.nxt as *mut BackTrace;

            if stk_ptr as u64 == 0 {
                break;
            }
        }
    }
}


/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    error!("\n!!!!! <- KERNEL PANIC -> !!!!!\n");
    if let Some(location) = info.location() {
        error!("  - Location ({}:{})\n",
            location.file(),
            location.line(),
        );
    }

    if let Some(mes) = info.message() {
        error!("  - Message: {:?}\n", mes);
    }

    BackTrace::decode(25);

    loop {}
}
