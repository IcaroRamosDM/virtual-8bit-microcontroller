#include "help.h"
#include "assets.h"
#include <FL/Fl_Box.H>
#include <FL/Fl.H>
#include <algorithm>
#include <cctype>

namespace vm8 {
static std::string lowercase(std::string text)
{
  std::transform(text.begin(), text.end(), text.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

HelpWindow::HelpWindow() : Fl_Double_Window(1080, 760, "VM8 Studio | Offline help")
{
  size_range(820, 560);
  entries_.push_back({"Start here", R"HELP(VM8 STUDIO - YOUR ASSEMBLY WORKBENCH

Everything you need is inside this application: an Assembly editor,
the two-pass assembler, the 8-bit CPU simulator, examples, and this help.
No terminal, GCC, Make, Ubuntu, Internet connection, or extra .bin file
is required to write and run your program.

TRY THE BUILT-IN POPCOUNT EXAMPLE
1. Choose Examples > Popcount.
2. Set Input to 0xA5 and click Apply.
3. Click Assemble, then Run (or simply Run to assemble automatically).
4. Expect OUT = 0x04, SP = 0x00, PC = 0x1C, and 126 cycles.
   At the default 1000000 Hz clock, Virtual time is 126.000 us.
   Real elapsed measures active simulation time on your computer.

Write a new program with File > New. Open and save .asm source files
through the File menu. Keep your own source files anywhere writable;
the executable's folder can be read-only.

Assembly mnemonics and registers are uppercase. Symbols are case-sensitive.
Use ; for comments. Put a label on its own line and finish it with :.
Byte values may be decimal (42) or hexadecimal (0x2A), from 0 to 255.

MINIMAL PROGRAM
    LDI A, 0x2A
    STA 0xEF
    HALT

This loads 42 into A and writes it to the output port. It occupies
five VM8 bytes and executes in three simulated instruction cycles.

Search at the top of this window to find instructions and concepts.
The instruction reference is embedded from the same help as the CLI.
)HELP"});
  entries_.push_back({"Controls and shortcuts", R"HELP(PROGRAM CONTROLS

Assemble (Ctrl+Enter)
  Translate the current editor contents into VM8 bytes. Success resets
  the CPU and loads the new program. Errors show the source line and
  reason. A failed assembly never runs an older program silently.

Run (F5)
  Assemble changed source if needed, then execute from the current PC.
  Execution stops at HALT, an error, a breakpoint, or the safety limit.
  After a breakpoint, Run continues past that stopped instruction.

Pause (F6)
  Stop automatic execution without changing CPU state. Step or Run
  can resume it. The window remains responsive during loops.

Step (F10)
  Execute one instruction, even when there is a breakpoint at that PC.
  Instructions may occupy one or two bytes, but count as one cycle.

Reset (Ctrl+R)
  Clear CPU state and reload the assembled program, applying the Input
  field. Both times return to zero; breakpoints and active clock remain.
  Source edits require a new assembly.

Toggle breakpoint (F9)
  Put the editor cursor on an instruction line and press F9.
  Execution stops BEFORE that instruction. Operands, comments, labels,
  and data-only lines are not independent instructions.
  Debug > Clear breakpoints removes every breakpoint.

Input / Apply
  Set the virtual byte at address 0xEE. Use 0..255 or 0x00..0xFF.
  Changing input does not reset the CPU. Apply is separate from editing
  the field. The byte is also applied when assembling or resetting.

Clock (Hz) / Apply
  Choose a decimal integer from 1 to 1000000000 Hz. Click its Apply
  button or press Enter in the field. Active shows the applied value.
  Recalculates Virtual time for all accumulated cycles, without resetting
  the CPU, altering Real elapsed, or changing execution speed.
  Run and Reset use the active clock, not unapplied text in the field.

Speed
  Observe, Normal and Fast execute 1, 16 and 128 instructions per GUI
  update, with a 20 ms scheduling delay. This is visualization pacing,
  not the CPU clock. See "Clock and execution time" for both timers.

Trace
  Include one row per attempted instruction in the event log.
  Log storage is bounded; old rows are discarded when it fills.

File / New (Ctrl+N), Open (Ctrl+O), Save (Ctrl+S), Save As (Ctrl+Shift+S)
  Work with .asm text files. Closing or replacing an edited document
  asks whether to save, discard, or cancel.
File / Export binary
  Save the assembled VM8 bytecode for use with the CLI or another VM8.
  This .bin is not a Windows/Linux executable.

Edit / Find (Ctrl+F) searches the source. F1 opens this offline help.
Memory shows live CPU memory and I/O. Bytes shows assembled bytes and
their source lines, so no wc or od command is needed in the GUI.

Safety: a single Run is limited to 10,000 attempted instructions.
Reaching the limit pauses execution; the program can still be inspected.
Changing the source pauses execution and invalidates its source mapping.
)HELP"});
  entries_.push_back({"Clock and execution time", R"HELP(TWO DIFFERENT MEASUREMENTS

VIRTUAL TIME
  Virtual time = attempted instruction cycles / active clock in Hz.
  Every attempted VM8 instruction counts as one cycle, including HALT
  and a failing opcode/stack operation. Instructions can occupy one or
  two bytes; their byte size does not change this cycle model.

  The default is 1000000 Hz (1 MHz): each cycle represents 1 us.
  Popcount with Input 0xA5 takes 126 cycles, so Virtual time is
  126 / 1000000 s = 126 us. Here "us" means microseconds.

  After running Popcount, set Clock (Hz) to 1000 and click its Apply.
  The same 126 cycles now show 126.000 ms. Registers, memory and Real
  elapsed stay unchanged. Change to 2000000 Hz for 63.000 us.
  Changing the clock recalculates ALL accumulated cycles using the new
  value; it does not model a history of frequency changes.
  This is a simple virtual timing model, not physical hardware timing
  or a promise to execute a million instructions per real second.

REAL ELAPSED
  A monotonic host stopwatch accumulates active Run intervals. It stops
  on Pause, HALT, errors, breakpoints, the safety limit, or source edits.
  Run resumes accumulation. Time spent paused or editing is excluded.
  Step adds the time for its CPU step and trace formatting; your wait
  between clicks and the subsequent screen refresh are not counted.

  During Run, elapsed time includes Speed delays and the GUI/trace work
  performed while running. It is NOT a CPU-only benchmark. It varies
  with your computer, system load, Speed and Trace settings.
  Turning Trace off and choosing Fast reduces visualization overhead,
  but still does not turn this measurement into a CPU-only benchmark.

RESET AND DISPLAY
  Reset, a successful assembly, New, Open, and loading an example clear
  both times. The active clock is retained for this application session.
  Failed assembly preserves the paused CPU/time snapshot but prevents
  running stale bytecode. Applying Input or clearing the log does not
  clear either timer. Restarting Studio restores the default clock.
  Values use ns (nanoseconds), us (microseconds), ms (milliseconds), or
  s (seconds). Displayed decimals are rounded, not an accuracy guarantee.

TRY IT
  Run Popcount with 0xA5 at 1 MHz using Observe, note both times, then
  Reset and run with Fast. Virtual time remains 126.000 us in both runs;
  Real elapsed will normally be shorter with Fast. Pausing and waiting
  before continuing must not add your waiting time to Real elapsed.
)HELP"});
  std::string reference = help_reference;
  const size_t instructions = reference.find("Supported instructions:\n");
  const size_t map = reference.find("Memory map:\n", instructions);
  std::vector<size_t> starts;
  for (size_t p = instructions; p != std::string::npos && p < map;) {
    const size_t end = reference.find('\n', p);
    if (reference.substr(p, end - p).find("Opcode:") != std::string::npos) starts.push_back(p);
    p = end == std::string::npos ? end : end + 1;
  }
  for (size_t i = 0; i < starts.size(); ++i) {
    const size_t end = i + 1 < starts.size() ? starts[i + 1] : map;
    std::string title = reference.substr(starts[i] + 2, reference.find("Opcode:", starts[i]) - starts[i] - 2);
    while (!title.empty() && title.back() == ' ') title.pop_back();
    entries_.push_back({title, reference.substr(starts[i], end - starts[i])});
  }
  const size_t directives = reference.find("Assembler directives:\n");
  const size_t workflow = reference.find("Simulator workflow:\n", directives);
  entries_.push_back({"Memory map", reference.substr(map, directives - map)});
  entries_.push_back({".EQU and .BYTE", reference.substr(directives, workflow - directives)});
  entries_.push_back({"Full command reference", std::string(
      "This is the embedded CLI reference. Its Make commands are for developers;\n"
      "use the Studio buttons for the equivalent actions, without installing Make.\n\n") + reference});
  entries_.push_back({"How the system works", system_guide});
  entries_.push_back({"About and licenses", std::string(
      "VM8 Studio - Virtual 8-bit Microcontroller\n\n"
      "Icaro Ramos Rodrigues dos Santos\n"
      "Embedded Systems | Firmware | C | Hardware Design | STM32 | ARM Cortex-M\n"
      "Linux | Verilog | UVM | Design Verification\n\n"
      "GitHub: https://github.com/IcaroRamosDM\n"
      "LinkedIn: https://www.linkedin.com/in/icaro-ramos-r/\n"
      "Contact: icaroelt@gmail.com\n\n"
      "The CPU and assembler are C17. The graphical layer is C++17.\n"
      "VM8 Studio is based in part on the work of the FLTK project\n"
      "(https://www.fltk.org), using FLTK 1.4.5 without source modifications.\n"
      "FLTK uses LGPL 2 with its static-linking exceptions.\n\n") + project_license});

  entries_.push_back({"Third-party notices", third_party_notices});
  begin();
  search_ = new Fl_Input(88, 16, w() - 104, 32, "Search");
  search_->when(FL_WHEN_CHANGED);
  search_->callback([](Fl_Widget *, void *p) { static_cast<HelpWindow *>(p)->filter(); }, this);
  topics_ = new Fl_Hold_Browser(16, 64, 236, h() - 80);
  topics_->textsize(14);
  topics_->selection_color(fl_rgb_color(29, 83, 79));
  topics_->callback([](Fl_Widget *, void *p) { static_cast<HelpWindow *>(p)->select(); }, this);
  display_ = new Fl_Text_Display(268, 64, w() - 284, h() - 80);
  display_->buffer(&buffer_);
  display_->textfont(FL_COURIER);
  display_->textsize(14);
  display_->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
  end();
  int screen_x, screen_y, screen_width, screen_height;
  Fl::screen_work_area(screen_x, screen_y, screen_width, screen_height, 0);
  resize(screen_x + 24, screen_y + 24, std::min(w(), screen_width - 48), std::min(h(), screen_height - 70));
  callback([](Fl_Widget *widget, void *) { widget->hide(); });
  filter();
}

HelpWindow::~HelpWindow()
{
  display_->buffer(nullptr);
}

void HelpWindow::resize(int x, int y, int width, int height)
{
  Fl_Double_Window::resize(x, y, width, height);
  search_->resize(88, 16, w() - 104, 32);
  topics_->resize(16, 64, 236, h() - 80);
  display_->resize(268, 64, w() - 284, h() - 80);
}

void HelpWindow::filter()
{
  const std::string term = lowercase(search_->value());
  topics_->clear();
  filtered_.clear();
  for (size_t i = 0; i < entries_.size(); ++i) {
    if (term.empty() || lowercase(entries_[i].title + "\n" + entries_[i].body).find(term) != std::string::npos) {
      topics_->add(entries_[i].title.c_str());
      filtered_.push_back(i);
    }
  }
  topics_->value(filtered_.empty() ? 0 : 1);
  select();
}

void HelpWindow::select()
{
  const int index = topics_->value() - 1;
  if (index < 0 || static_cast<size_t>(index) >= filtered_.size()) {
    buffer_.text("No matching topic. Try a mnemonic such as LDI, a flag, or memory.");
    return;
  }
  buffer_.text(entries_[filtered_[static_cast<size_t>(index)]].body.c_str());
  display_->scroll(0, 0);
}

void HelpWindow::open()
{
  show();
  search_->take_focus();
}
}
