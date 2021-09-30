pub struct SimpleLogger;
pub struct UartLogger;

use log::{Record, Level, Metadata, LevelFilter};
use core::fmt::Write;
use lazy_static::lazy_static;
use spin::Mutex;


#[path = "arch/x86_64/mod.rs"]
pub mod arch;

static LOGGER: SimpleLogger = SimpleLogger;

lazy_static! {
    static ref ULOGGER: Mutex<UartLogger> = Mutex::new(UartLogger);
}

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
            write!(ULOGGER.lock(), "{}", record.args());
        }
    }
    
    // FLushing is not really required
    fn flush(&self) {}
}

pub fn init() {
    log::set_logger(&LOGGER)
        .map(|()| log::set_max_level(LevelFilter::Info));
}
