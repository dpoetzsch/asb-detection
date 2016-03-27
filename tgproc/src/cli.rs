extern crate argparse;

use self::argparse::{ArgumentParser, StoreTrue, StoreFalse, Store, Collect};

pub struct Options {
    pub taintgrind_trace: bool,
    pub mark_taint: bool,
    pub mark_trace: bool,
    pub libs: bool,
    pub tmp_instr: bool,
    pub unique_locs: bool,
    pub src_only: bool,
    pub color: bool,
    pub verbose: bool,
    pub sink_lines: Vec<i32>,
    pub logfile: String
}

pub fn parse_opts() -> Options {
    // parse cli options
    let mut cli_opts = Options {
        taintgrind_trace: false,
        mark_taint: false,
        mark_trace: false,
        libs: false,
        tmp_instr: false,
        unique_locs: false,
        src_only: false,
        color: true,
        verbose: false,
        sink_lines: vec![],
        logfile: "".to_string()
    };
    
    {
        let mut ap = ArgumentParser::new();
        
        ap.set_description("This finds traces in the taintgrind output that lead to \
                            dangerous behavior.");
        
        ap.refer(&mut cli_opts.verbose)
            .add_option(&["-v", "--verbose"], StoreTrue,
                        "Verbose mode");
        
        ap.refer(&mut cli_opts.libs)
            .add_option(&["--libs"], StoreTrue,
                        "In the traces, show lines that are located in 3rd-party-libraries. \
                         Specifically, a line is considered to be in a \
                         library, if the source file cannot be found below the \
                         current working directory.");
        
        ap.refer(&mut cli_opts.tmp_instr)
            .add_option(&["--tmp-instr"], StoreTrue,
                        "In the traces, show lines that affect only temporary \
                         variables inserted by valgrind.");
        
        ap.refer(&mut cli_opts.unique_locs)
            .add_option(&["--unique-locs"], StoreTrue,
                        "In the traces, don't show the same source location twice \
                         (e.g. in a loop). This makes the trace incomplete but \
                         avoids very big traces.");
        
        ap.refer(&mut cli_opts.src_only)
            .add_option(&["--src-only"], StoreTrue,
                        "Show only the sources, not the full trace.");
        
        ap.refer(&mut cli_opts.taintgrind_trace)
            .add_option(&["--taintgrind-trace"], StoreTrue,
                        "Show the taintgrind trace for the identified sinks.");
        
        ap.refer(&mut cli_opts.mark_trace)
            .add_option(&["--mark-trace"], StoreTrue,
                        "For each trace print the whole taintgrind log but mark \
                         the trace using color");
        
        ap.refer(&mut cli_opts.mark_taint)
            .add_option(&["--mark-taint"], StoreTrue,
                        "Just mark the taint color of each line");
        
        ap.refer(&mut cli_opts.color)
            .add_option(&["--no-color"], StoreFalse,
                        "Do not use terminal colors");
        
        ap.refer(&mut cli_opts.sink_lines)
            .add_option(&["--mark-sink"], Collect,
                        "Mark the line as sink; this disables automatic sink detection")
            .metavar("lineno");

        ap.refer(&mut cli_opts.logfile)
            .add_argument("<taintgrind log>", Store,
                          "The taintgrind log file")
            .required();
        
        ap.parse_args_or_exit();
    }
    cli_opts
}