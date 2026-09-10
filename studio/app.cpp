#include "app.h"
#include "assets.h"
#include <FL/Fl.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Image_Surface.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/fl_utf8.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace vm8 {
namespace {
constexpr int MARGIN = 18, GAP = 14, TOOLBAR_Y = 100, CONTENT_Y = 179;
constexpr int LOG_HEIGHT = 187, FOOTER_HEIGHT = 34;
constexpr int LOG_CAPACITY = 65536;
constexpr double TIMER_SECONDS = 0.02;
const Fl_Color BACKGROUND = fl_rgb_color(17, 23, 34);
const Fl_Color PANEL = fl_rgb_color(25, 34, 48);
const Fl_Color EDITOR = fl_rgb_color(12, 18, 27);
const Fl_Color FOREGROUND = fl_rgb_color(225, 233, 242);
const Fl_Color MUTED = fl_rgb_color(143, 160, 181);
const Fl_Color ACCENT = fl_rgb_color(64, 213, 190);
const Fl_Color BLUE = fl_rgb_color(124, 188, 255);
const Fl_Color YELLOW = fl_rgb_color(240, 206, 128);
const Fl_Color RED = fl_rgb_color(255, 126, 126);
const Fl_Text_Display::Style_Table_Entry SYNTAX[] = {
  {FOREGROUND, FL_COURIER, 16, 0, EDITOR},
  {MUTED, FL_COURIER_ITALIC, 16, 0, EDITOR},
  {BLUE, FL_COURIER_BOLD, 16, 0, EDITOR},
  {YELLOW, FL_COURIER, 16, 0, EDITOR},
  {ACCENT, FL_COURIER, 16, 0, EDITOR},
  {FOREGROUND, FL_COURIER_BOLD, 16, Fl_Text_Display::ATTR_BGCOLOR_EXT, fl_rgb_color(29, 68, 66)},
  {FOREGROUND, FL_COURIER_BOLD, 16, Fl_Text_Display::ATTR_BGCOLOR_EXT, fl_rgb_color(72, 45, 48)},
};

std::string text_of(const Fl_Text_Buffer &buffer)
{
  char *text = buffer.text();
  std::string result(text ? text : "");
  std::free(text);
  return result;
}

void format_display(Fl_Text_Display *display, Fl_Text_Buffer *buffer, int size)
{
  display->buffer(buffer);
  display->box(FL_FLAT_BOX);
  display->color(EDITOR);
  display->textcolor(FOREGROUND);
  display->textfont(FL_COURIER);
  display->textsize(size);
  display->selection_color(fl_rgb_color(45, 77, 107));
}

Fl_Box *heading(const char *text, int size = 12, Fl_Color color = MUTED)
{
  auto *box = new Fl_Box(0, 0, 1, 1, text);
  box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  box->labelfont(FL_HELVETICA_BOLD);
  box->labelsize(size);
  box->labelcolor(color);
  return box;
}

bool capture(Fl_Window &window, const std::string &path)
{
  Fl::check();
  Fl::flush();
  // Render this application's widgets, including when a Windows smoke test is hidden.
  Fl_Image_Surface surface(window.w(), window.h());
  Fl_Surface_Device::push_current(&surface);
  surface.draw(&window);
  std::unique_ptr<Fl_RGB_Image> image(surface.image());
  Fl_Surface_Device::pop_current();
  return image && fl_write_png(path.c_str(), image.get()) == 0;
}
}

void configure_theme()
{
  Fl::scheme("gtk+");
#ifndef _WIN32
  // Use the bundled chooser instead of optional GTK/KDE helper processes.
  Fl::option(Fl::OPTION_FNFC_USES_GTK, false);
  Fl::option(Fl::OPTION_FNFC_USES_ZENITY, false);
  Fl::option(Fl::OPTION_FNFC_USES_KDIALOG, false);
#endif
  Fl::background(25, 34, 48);
  Fl::background2(12, 18, 27);
  Fl::foreground(225, 233, 242);
#ifdef _WIN32
  Fl::set_font(FL_COURIER, " Consolas");
  Fl::set_font(FL_COURIER_BOLD, "BConsolas");
  Fl::set_font(FL_COURIER_ITALIC, "IConsolas");
  Fl::set_font(FL_HELVETICA, " Segoe UI");
  Fl::set_font(FL_HELVETICA_BOLD, "BSegoe UI");
#endif
  Fl::visible_focus(1);
  fl_message_font(FL_HELVETICA, 15);
}

App::App() : Fl_Double_Window(1480, 920, "VM8 Studio")
{
  size_range(900, 580);
  color(BACKGROUND);
  begin();
  brand_ = heading("VM8  /  STUDIO", 26, FOREGROUND);
  subtitle_ = heading("ASSEMBLY WORKBENCH   /   8-BIT CPU   /   OFFLINE", 11, ACCENT);
  menu_ = new Fl_Menu_Bar(MARGIN, 64, w() - 2 * MARGIN, 26);
  menu_->box(FL_FLAT_BOX);
  menu_->color(BACKGROUND);
  menu_->textsize(14);
  const auto menu = [this](const char *label, int shortcut, Action action, int flags = 0) {
    menu_->add(label, shortcut, action_callback, reinterpret_cast<void *>(static_cast<intptr_t>(action)), flags);
  };
  menu("&File/&New", FL_CTRL + 'n', Action::New);
  menu("&File/&Open...", FL_CTRL + 'o', Action::Open);
  menu("&File/&Save", FL_CTRL + 's', Action::Save);
  menu("&File/Save &as...", FL_CTRL + FL_SHIFT + 's', Action::SaveAs, FL_MENU_DIVIDER);
  menu("&File/Export &binary...", 0, Action::Export, FL_MENU_DIVIDER);
  menu("&File/&Quit", FL_ALT + FL_F + 4, Action::Quit);
  menu("&Edit/&Find...", FL_CTRL + 'f', Action::Find);
  menu("&Debug/&Assemble", FL_CTRL + FL_Enter, Action::Assemble);
  menu("&Debug/&Run", FL_F + 5, Action::Run);
  menu("&Debug/&Pause", FL_F + 6, Action::Pause);
  menu("&Debug/&Step", FL_F + 10, Action::Step);
  menu("&Debug/Reset &CPU", FL_CTRL + 'r', Action::Reset, FL_MENU_DIVIDER);
  menu("&Debug/Toggle &breakpoint", FL_F + 9, Action::Breakpoint);
  menu("&Debug/&Clear breakpoints", 0, Action::ClearBreakpoints);
  menu("E&xamples/&Introductory demo", 0, Action::Demo);
  menu("E&xamples/&Popcount", 0, Action::Popcount);
  menu("&Help/&Offline reference", FL_F + 1, Action::Help);

  const char *labels[] = {"Assemble", "Run  /  F5", "Pause", "Step  /  F10", "Reset", "Breakpoint", "Help  /  F1"};
  const Action actions[] = {Action::Assemble, Action::Run, Action::Pause, Action::Step,
                            Action::Reset, Action::Breakpoint, Action::Help};
  for (size_t i = 0; i < controls_.size(); ++i) {
    controls_[i] = new Fl_Button(0, TOOLBAR_Y, 100, 36, labels[i]);
    controls_[i]->box(FL_FLAT_BOX);
    controls_[i]->color(i < 2 ? ACCENT : PANEL);
    controls_[i]->labelcolor(i < 2 ? EDITOR : FOREGROUND);
    controls_[i]->labelfont(FL_HELVETICA_BOLD);
    controls_[i]->labelsize(13);
    controls_[i]->callback(action_callback, reinterpret_cast<void *>(static_cast<intptr_t>(actions[i])));
  }
  editor_title_ = heading("SOURCE / ASSEMBLY");
  cpu_title_ = heading("LIVE CPU / REGISTERS AND PORTS");
  editor_ = new Fl_Text_Editor(0, 0, 1, 1);
  format_display(editor_, &source_, 16);
  source_.tab_distance(4);
  editor_->linenumber_width(48);
  editor_->linenumber_font(FL_COURIER);
  editor_->linenumber_size(13);
  editor_->linenumber_fgcolor(MUTED);
  editor_->linenumber_bgcolor(PANEL);
  editor_->cursor_color(ACCENT);
  editor_->highlight_data(&styles_, SYNTAX, sizeof SYNTAX / sizeof SYNTAX[0], 'A', nullptr, nullptr);
  source_.add_modify_callback([](int, int inserted, int deleted, int, const char *, void *p) {
    if (inserted || deleted) static_cast<App *>(p)->changed();
  }, this);
  log_title_ = heading("EVENT LOG / EXECUTION TRACE");
  trace_ = new Fl_Check_Button(0, 0, 80, 25, "Trace");
  trace_->value(1);
  trace_->labelsize(12);
  trace_->selection_color(ACCENT);
  clear_log_ = new Fl_Button(0, 0, 64, 25, "Clear");
  clear_log_->box(FL_FLAT_BOX);
  clear_log_->callback(action_callback, reinterpret_cast<void *>(static_cast<intptr_t>(Action::ClearLog)));
  log_display_ = new Fl_Text_Display(0, 0, 1, 1);
  format_display(log_display_, &log_, 13);
  state_display_ = new Fl_Text_Display(0, 0, 1, 1);
  format_display(state_display_, &state_, 18);
  timing_display_ = new Fl_Text_Display(0, 0, 1, 1);
  format_display(timing_display_, &timing_, 15);
  timing_display_->textcolor(ACCENT);
  timing_display_->tooltip("Virtual time = cycles / active clock. Real elapsed = active Run/Step time, excluding pauses. Press F1 for details.");
  input_ = new Fl_Input(0, 0, 86, 32, "Input (0xEE)");
  input_->value("0xA5");
  input_->labelsize(12);
  input_->textfont(FL_COURIER);
  input_->textsize(15);
  input_->maximum_size(5);
  apply_ = new Fl_Button(0, 0, 65, 32, "Apply");
  apply_->box(FL_FLAT_BOX);
  apply_->callback(action_callback, reinterpret_cast<void *>(static_cast<intptr_t>(Action::Apply)));
  speed_ = new Fl_Choice(0, 0, 130, 32, "Speed");
  speed_->labelsize(12);
  speed_->textsize(12);
  speed_->add("Observe|Normal|Fast");
  speed_->value(1);
  speed_->tooltip("Visualization rate, not the virtual CPU clock. Slower pacing increases real elapsed time only.");
  clock_input_ = new Fl_Input(0, 0, 136, 32, "Clock (Hz)");
  clock_input_->value(std::to_string(DEFAULT_CLOCK_HZ).c_str());
  clock_input_->labelsize(12);
  clock_input_->textfont(FL_COURIER);
  clock_input_->textsize(15);
  clock_input_->maximum_size(32);
  clock_input_->tooltip("Decimal integer, 1..1000000000 Hz. Click Apply or press Enter. Recalculates virtual time without changing execution speed.");
  clock_input_->when(FL_WHEN_ENTER_KEY_ALWAYS);
  clock_input_->callback(action_callback, reinterpret_cast<void *>(static_cast<intptr_t>(Action::ApplyClock)));
  apply_clock_ = new Fl_Button(0, 0, 65, 32, "Apply");
  apply_clock_->box(FL_FLAT_BOX);
  apply_clock_->callback(action_callback, reinterpret_cast<void *>(static_cast<intptr_t>(Action::ApplyClock)));
  active_clock_ = heading("", 12, ACCENT);
  tabs_ = new Fl_Tabs(0, 0, 1, 1);
  memory_page_ = new Fl_Group(0, 0, 1, 1, "Live memory");
  memory_display_ = new Fl_Text_Display(0, 0, 1, 1);
  format_display(memory_display_, &memory_, 14);
  memory_page_->end();
  binary_page_ = new Fl_Group(0, 0, 1, 1, "Assembled bytes");
  binary_display_ = new Fl_Text_Display(0, 0, 1, 1);
  format_display(binary_display_, &binary_, 14);
  binary_page_->end();
  tabs_->end();
  tabs_->selection_color(PANEL);
  status_ = heading("", 12, ACCENT);
  end();
  callback([](Fl_Widget *widget, void *) { static_cast<App *>(widget)->act(Action::Quit); });
  layout();
  int screen_x, screen_y, screen_width, screen_height;
  Fl::screen_work_area(screen_x, screen_y, screen_width, screen_height, 0);
  resize(screen_x + 12, screen_y + 12, std::min(w(), screen_width - 24), std::min(h(), screen_height - 50));
  set_source(popcount_source, "", false);
  append_log("Welcome to VM8 Studio. Popcount is ready to assemble.\n"
             "Input 0xA5 contains four set bits: run it and expect OUT = 0x04.\n"
             "F1 opens the complete offline reference.\n");
  set_status("Ready / Popcount example / Assemble or press F5 to begin");
}

App::~App()
{
  Fl::remove_timeout(timer, this);
  editor_->highlight_data(nullptr, nullptr, 0, 0, nullptr, nullptr);
  editor_->buffer(nullptr);
  log_display_->buffer(nullptr);
  state_display_->buffer(nullptr);
  timing_display_->buffer(nullptr);
  memory_display_->buffer(nullptr);
  binary_display_->buffer(nullptr);
}

int App::handle(int event)
{
  // Editor key bindings must not consume the workbench's advertised shortcuts.
  if (event == FL_KEYDOWN) {
    const int key = Fl::event_key();
    const int modifiers = Fl::event_state() & (FL_CTRL | FL_ALT | FL_SHIFT | FL_META);
    if (modifiers == 0) {
      switch (key) {
        case FL_F + 1: act(Action::Help); return 1;
        case FL_F + 5: act(Action::Run); return 1;
        case FL_F + 6: act(Action::Pause); return 1;
        case FL_F + 9: act(Action::Breakpoint); return 1;
        case FL_F + 10: act(Action::Step); return 1;
        default: break;
      }
    }
    if (modifiers == FL_CTRL) {
      switch (key) {
        case FL_Enter: act(Action::Assemble); return 1;
        case 'n': act(Action::New); return 1;
        case 'o': act(Action::Open); return 1;
        case 's': act(Action::Save); return 1;
        case 'r': act(Action::Reset); return 1;
        case 'f': act(Action::Find); return 1;
        default: break;
      }
    }
    if (modifiers == (FL_CTRL | FL_SHIFT) && key == 's') { act(Action::SaveAs); return 1; }
  }
  return Fl_Double_Window::handle(event);
}

void App::resize(int x, int y, int width, int height)
{
  Fl_Double_Window::resize(x, y, width, height);
  layout();
}

void App::layout()
{
  const int right_width = std::max(490, w() * 38 / 100);
  const int left_width = w() - right_width - 2 * MARGIN - GAP;
  const int right = MARGIN + left_width + GAP;
  const int bottom = h() - FOOTER_HEIGHT;
  const int log_height = std::min(LOG_HEIGHT, h() / 4);
  const int log_top = bottom - log_height;
  brand_->resize(MARGIN, 12, 330, 37);
  subtitle_->resize(370, 19, w() - 390, 26);
  menu_->resize(MARGIN, 64, w() - 2 * MARGIN, 26);
  int x = MARGIN;
  for (auto *control : controls_) {
    const int width = 110;
    control->resize(x, TOOLBAR_Y, width, 36);
    x += width + 8;
  }
  editor_title_->resize(MARGIN, 149, left_width, 22);
  cpu_title_->resize(right, 149, right_width, 22);
  editor_->resize(MARGIN, CONTENT_Y, left_width, log_top - CONTENT_Y - 38);
  log_title_->resize(MARGIN, log_top - 30, left_width - 170, 22);
  trace_->resize(MARGIN + left_width - 156, log_top - 31, 80, 25);
  clear_log_->resize(MARGIN + left_width - 64, log_top - 31, 64, 25);
  log_display_->resize(MARGIN, log_top, left_width, log_height - 8);
  state_display_->resize(right, CONTENT_Y, right_width, 110);
  timing_display_->resize(right, CONTENT_Y + 112, right_width, 50);
  input_->resize(right + 88, CONTENT_Y + 169, 83, 32);
  apply_->resize(right + 179, CONTENT_Y + 169, 65, 32);
  speed_->resize(right + right_width - 127, CONTENT_Y + 169, 127, 32);
  clock_input_->resize(right + 88, CONTENT_Y + 211, 136, 32);
  apply_clock_->resize(right + 232, CONTENT_Y + 211, 65, 32);
  active_clock_->resize(right + 309, CONTENT_Y + 211, right_width - 309, 32);
  const int tabs_top = CONTENT_Y + 254;
  tabs_->resize(right, tabs_top, right_width, bottom - tabs_top - 8);
  for (auto *page : {memory_page_, binary_page_})
    page->resize(right, tabs_top + 29, right_width, bottom - tabs_top - 37);
  for (auto *display : {memory_display_, binary_display_})
    display->resize(right + 2, tabs_top + 31, right_width - 4, bottom - tabs_top - 41);
  status_->resize(MARGIN, bottom + 3, w() - 2 * MARGIN, 24);
}

void App::set_status(const std::string &text)
{
  status_text_ = text;
  status_->copy_label(status_text_.c_str());
}

void App::append_log(const std::string &text)
{
  log_.append(text.c_str());
  if (log_.length() > LOG_CAPACITY) {
    const int end = log_.line_end(log_.length() - LOG_CAPACITY) + 1;
    log_.remove(0, std::min(end, log_.length()));
  }
  log_display_->insert_position(log_.length());
  log_display_->show_insert_position();
}

void App::set_source(const std::string &text, const std::string &path, bool modified)
{
  pause("");
  loading_ = true;
  source_.text(text.c_str());
  loading_ = false;
  dirty_ = modified;
  path_ = path;
  machine_.assembled = false;
  machine_.breakpoints.fill(false);
  machine_.image = AssemblyImage{};
  cpu_reset(&machine_.cpu);
  execution_timer_.reset();
  error_line_ = 0;
  editor_->insert_position(0);
  editor_->scroll(0, 0);
  refresh();
}

void App::changed()
{
  if (loading_) return;
  pause("");
  dirty_ = true;
  machine_.assembled = false;
  error_line_ = 0;
  set_status("Source changed / Assemble to update the program and source mapping");
  refresh();
}

void App::refresh()
{
  std::string title = "VM8 Studio / " + (path_.empty() ? std::string("Untitled.asm") : path_);
  if (dirty_) title += " *";
  copy_label(title.c_str());
  const Cpu &cpu = machine_.cpu;
  char state[512];
  char bits[9]{};
  const unsigned output = cpu_get_output_port(&cpu);
  for (unsigned bit = 0; bit < 8; ++bit) bits[bit] = (output & (1U << (7 - bit))) ? '1' : '0';
  std::snprintf(state, sizeof state,
      " A   0x%02X    B   0x%02X    PC  0x%02X\n"
      " SP  0x%02X    Z   %s     C   %s\n"
      " IN  0x%02X    OUT 0x%02X    %s\n"
      " Cycles: %-7llu   %s",
      static_cast<unsigned>(cpu.register_a), static_cast<unsigned>(cpu.register_b),
      static_cast<unsigned>(cpu.program_counter), static_cast<unsigned>(cpu.stack_pointer),
      cpu.zero_flag ? "set  " : "clear", cpu.carry_flag ? "set" : "clear",
      static_cast<unsigned>(cpu_read_memory(&cpu, CPU_INPUT_PORT_ADDRESS)), output, bits,
      static_cast<unsigned long long>(cpu.cycle_count), running_ ? "RUNNING" : cpu.halted ? "HALTED" : "PAUSED");
  state_.text(state);
  const std::string timing = " Virtual time: " + format_seconds(virtual_seconds(cpu.cycle_count, clock_hz_)) +
      "\n Real elapsed: " + format_seconds(execution_timer_.seconds());
  timing_.text(timing.c_str());
  active_clock_->copy_label(("Active: " + std::to_string(clock_hz_) + " Hz").c_str());
  memory_.text(machine_.memory_text().c_str());
  binary_.text(machine_.assembled ? machine_.binary_text().c_str() : "Assemble the current source to inspect its bytes.\n");
  char caption[128];
  std::snprintf(caption, sizeof caption, "SOURCE / ASSEMBLY%s%s",
                machine_.assembled ? " / " : "", machine_.assembled ? "BYTECODE READY" : "");
  editor_title_->copy_label(caption);
  if (running_) controls_[2]->activate(); else controls_[2]->deactivate();
  color_source();
}

void App::color_source()
{
  const std::string text = text_of(source_);
  std::string style(text.size(), 'A');
  size_t line = 1;
  for (size_t p = 0; p < text.size();) {
    const size_t newline = text.find('\n', p);
    const size_t end = newline == std::string::npos ? text.size() : newline;
    size_t comment = text.find(';', p);
    if (comment == std::string::npos || comment > end) comment = end;
    std::fill(style.begin() + static_cast<std::ptrdiff_t>(comment), style.begin() + static_cast<std::ptrdiff_t>(end), 'B');
    for (size_t token = p; token < comment;) {
      if (std::isspace(static_cast<unsigned char>(text[token])) || text[token] == ',') { ++token; continue; }
      size_t last = token + 1;
      while (last < comment && !std::isspace(static_cast<unsigned char>(text[last])) && text[last] != ',') ++last;
      const std::string word = text.substr(token, last - token);
      const char color = std::isdigit(static_cast<unsigned char>(word[0])) ? 'D' :
                         word.back() == ':' ? 'E' : word[0] == '.' ? 'E' : 'C';
      std::fill(style.begin() + static_cast<std::ptrdiff_t>(token), style.begin() + static_cast<std::ptrdiff_t>(last), color);
      token = last;
    }
    char line_style = line == error_line_ ? 'G' : 0;
    if (machine_.assembled) {
      for (size_t address = 0; address < machine_.image.byte_count; ++address) {
        if (machine_.image.instruction_start[address] && machine_.image.line_for_address[address] == line) {
          if (machine_.breakpoints[address]) line_style = 'G';
          if (!machine_.cpu.halted && address == machine_.cpu.program_counter) line_style = 'F';
          break;
        }
      }
    }
    if (line_style) std::fill(style.begin() + static_cast<std::ptrdiff_t>(p), style.begin() + static_cast<std::ptrdiff_t>(end), line_style);
    p = end == text.size() ? end : end + 1;
    ++line;
  }
  styles_.text(style.c_str());
  editor_->redisplay_range(0, source_.length());
}

bool App::get_input(uint8_t &value)
{
  if (byte_value_parse(input_->value(), &value)) return true;
  append_log("Input rejected: use a decimal byte (0..255) or hexadecimal byte (0x00..0xFF).\n");
  set_status("Invalid input / CPU state was not changed");
  input_->take_focus();
  return false;
}

void App::apply_clock()
{
  if (!parse_clock_hz(clock_input_->value(), clock_hz_)) {
    append_log("Clock rejected: use a decimal integer from 1 to 1000000000 Hz, without units or separators.\n");
    set_status("Invalid clock / Active frequency, CPU state and timers were not changed");
    clock_input_->take_focus();
    return;
  }
  clock_input_->value(std::to_string(clock_hz_).c_str());
  set_status("Clock applied / Virtual time recalculated / Speed and real elapsed unchanged");
  refresh();
}

void App::reveal_line(size_t line)
{
  if (!line) return;
  const int position = source_.skip_lines(0, static_cast<int>(line - 1));
  editor_->insert_position(position);
  editor_->show_insert_position();
}

bool App::assemble()
{
  pause("");
  uint8_t input;
  if (!get_input(input)) return false;
  const bool success = machine_.assemble(text_of(source_), input);
  error_line_ = success ? 0 : machine_.image.error_line;
  if (!success) {
    append_log(machine_.messages);
    set_status("Assembly failed / Check the event log and highlighted source line");
    reveal_line(error_line_);
  } else {
    execution_timer_.reset();
    char message[160];
    std::snprintf(message, sizeof message, "Assembled: %u / %u bytes, %u instructions, %u symbols. CPU reset.\n",
        static_cast<unsigned>(machine_.image.byte_count), static_cast<unsigned>(CPU_PROGRAM_MEMORY_SIZE),
        static_cast<unsigned>(machine_.image.instruction_count), static_cast<unsigned>(machine_.image.symbol_count));
    append_log(message);
    set_status(message);
    breakpoint_stop_ = false;
  }
  refresh();
  return success;
}

bool App::ensure_assembled()
{
  return machine_.assembled || assemble();
}

void App::pause(const std::string &reason)
{
  const bool was_running = running_;
  running_ = false;
  execution_timer_.stop();
  Fl::remove_timeout(timer, this);
  if (!reason.empty()) { append_log(reason + "\n"); set_status(reason); }
  if (was_running) refresh();
}

void App::run()
{
  if (running_ || !ensure_assembled()) return;
  if (machine_.cpu.halted) {
    set_status("CPU is halted / Reset to run the program again");
    return;
  }
  skip_breakpoint_ = breakpoint_stop_;
  breakpoint_stop_ = false;
  run_count_ = 0;
  running_ = true;
  execution_timer_.start();
  set_status("Running / F6 pauses / 10,000-instruction safety limit");
  refresh();
  Fl::add_timeout(TIMER_SECONDS, timer, this);
}

void App::timer(void *context) { static_cast<App *>(context)->tick(); }

void App::tick()
{
  if (!running_) return;
  constexpr unsigned BATCH_SIZES[] = {1, 16, 128};
  const unsigned batch = BATCH_SIZES[std::clamp(speed_->value(), 0, 2)];
  std::string rows;
  for (unsigned i = 0; i < batch && running_; ++i) {
    const uint8_t pc = machine_.cpu.program_counter;
    if (machine_.breakpoints[pc] && !skip_breakpoint_) {
      breakpoint_stop_ = true;
      pause("Paused before breakpoint / Step or Run to continue");
      reveal_line(machine_.image.line_for_address[pc]);
      break;
    }
    skip_breakpoint_ = false;
    std::string row;
    const auto result = machine_.step(trace_->value() ? &row : nullptr);
    rows += row;
    ++run_count_;
    if (result != CPU_STEP_OK) {
      append_log(rows);
      rows.clear();
      pause(std::string("Execution result: ") + Machine::result_name(result));
    } else if (run_count_ == RUN_INSTRUCTION_LIMIT) {
      append_log(rows);
      rows.clear();
      pause("Paused: 10,000-instruction safety limit reached / Inspect the loop or Run to continue");
    }
  }
  if (!rows.empty()) append_log(rows);
  refresh();
  if (running_) Fl::add_timeout(TIMER_SECONDS, timer, this);
}

void App::step()
{
  pause("");
  if (!ensure_assembled()) return;
  if (machine_.cpu.halted) { set_status("CPU is halted / Reset before stepping"); return; }
  breakpoint_stop_ = false;
  std::string row;
  execution_timer_.start();
  const auto result = machine_.step(&row);
  execution_timer_.stop();
  append_log(row);
  set_status(std::string("Step: ") + Machine::result_name(result));
  if (!machine_.cpu.halted && machine_.cpu.program_counter < CPU_PROGRAM_MEMORY_SIZE)
    reveal_line(machine_.image.line_for_address[machine_.cpu.program_counter]);
  refresh();
}

void App::toggle_breakpoint()
{
  const int position = editor_->insert_position();
  if (!ensure_assembled()) return;
  const size_t line = static_cast<size_t>(source_.count_lines(0, position)) + 1;
  for (size_t address = 0; address < machine_.image.byte_count; ++address) {
    if (machine_.image.instruction_start[address] && machine_.image.line_for_address[address] == line) {
      machine_.breakpoints[address] = !machine_.breakpoints[address];
      char message[100];
      std::snprintf(message, sizeof message, "Breakpoint %s at 0x%02X (source line %u)",
          machine_.breakpoints[address] ? "set" : "removed", static_cast<unsigned>(address), static_cast<unsigned>(line));
      append_log(std::string(message) + "\n");
      set_status(message);
      refresh();
      return;
    }
  }
  set_status("Choose a line containing an instruction to set a breakpoint");
}

bool App::save_to(const std::string &path)
{
  const std::string text = text_of(source_);
  FILE *file = fl_fopen(path.c_str(), "wb");
  if (!file) { fl_alert("Cannot open this location for writing.\n%s", path.c_str()); return false; }
  const bool written = std::fwrite(text.data(), 1, text.size(), file) == text.size();
  const bool closed = std::fclose(file) == 0;
  if (!written || !closed) { fl_alert("Could not finish saving this file.\n%s", path.c_str()); return false; }
  path_ = path;
  dirty_ = false;
  set_status("Saved / " + path_);
  refresh();
  return true;
}

bool App::save(bool choose_path)
{
  if (!choose_path && !path_.empty()) return save_to(path_);
  Fl_Native_File_Chooser chooser;
  chooser.title("Save Assembly source");
  chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
  chooser.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM | Fl_Native_File_Chooser::USE_FILTER_EXT);
  chooser.filter("Assembly source\t*.asm");
  chooser.preset_file(path_.empty() ? "program.asm" : path_.c_str());
  if (chooser.show() != 0) return false;
  return save_to(chooser.filename());
}

bool App::may_replace()
{
  if (!dirty_) return true;
  pause("");
  const int answer = fl_choice("Save your changes before continuing?", "Cancel", "Save", "Discard");
  if (answer == 1) return save(false);
  return answer == 2;
}

bool App::read_from(const std::string &path)
{
  FILE *file = fl_fopen(path.c_str(), "rb");
  if (!file) { fl_alert("Cannot open source file.\n%s", path.c_str()); return false; }
  std::string text(static_cast<size_t>(ASSEMBLY_SOURCE_CAPACITY) + 1, '\0');
  const size_t size = std::fread(text.data(), 1, text.size(), file);
  const bool error = std::ferror(file) != 0;
  std::fclose(file);
  text.resize(size);
  if (error || size > ASSEMBLY_SOURCE_CAPACITY || text.find('\0') != std::string::npos) {
    fl_alert("Use a text Assembly file of at most 64 KiB, without NUL bytes.");
    return false;
  }
  if (text.compare(0, 3, "\xEF\xBB\xBF") == 0) text.erase(0, 3);
  text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
  set_source(text, path, false);
  set_status("Opened / " + path);
  return true;
}

void App::open()
{
  pause("");
  if (!may_replace()) return;
  Fl_Native_File_Chooser chooser;
  chooser.title("Open Assembly source");
  chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
  chooser.filter("Assembly source\t*.asm\nText files\t*.txt");
  if (chooser.show() == 0) (void)read_from(chooser.filename());
}

void App::export_binary()
{
  pause("");
  if (!ensure_assembled()) return;
  Fl_Native_File_Chooser chooser;
  chooser.title("Export VM8 bytecode (not a desktop executable)");
  chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
  chooser.filter("VM8 binary\t*.bin");
  chooser.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM | Fl_Native_File_Chooser::USE_FILTER_EXT);
  chooser.preset_file("program.bin");
  if (chooser.show() != 0) return;
  FILE *file = fl_fopen(chooser.filename(), "wb");
  if (!file) { fl_alert("Cannot create the binary file at this location."); return; }
  const bool written = std::fwrite(machine_.image.bytes, 1, machine_.image.byte_count, file) == machine_.image.byte_count;
  const bool closed = std::fclose(file) == 0;
  if (!written || !closed) fl_alert("Could not finish writing the binary file.");
  else set_status(std::string("Exported VM8 bytecode / ") + chooser.filename());
}

void App::action_callback(Fl_Widget *widget, void *data)
{
  static_cast<App *>(widget->window())->act(static_cast<Action>(reinterpret_cast<intptr_t>(data)));
}

void App::act(Action action)
{
  switch (action) {
    case Action::New:
      if (may_replace()) { set_source("; Write your VM8 Assembly program here.\n\nHALT\n", "", false); set_status("New source / Untitled.asm"); }
      break;
    case Action::Open: open(); break;
    case Action::Save: (void)save(false); break;
    case Action::SaveAs: (void)save(true); break;
    case Action::Export: export_binary(); break;
    case Action::Quit:
      if (may_replace()) { pause(""); help_.hide(); hide(); }
      break;
    case Action::Find: {
      pause("");
      const char *term = fl_input("Find in Assembly source (case-sensitive):", "");
      if (!term || !*term) break;
      int position;
      if (source_.search_forward(editor_->insert_position(), term, &position, 1) || source_.search_forward(0, term, &position, 1)) {
        source_.select(position, position + static_cast<int>(std::strlen(term)));
        editor_->insert_position(position + static_cast<int>(std::strlen(term)));
        editor_->show_insert_position();
      } else set_status(std::string("Text not found / ") + term);
      break;
    }
    case Action::Assemble: (void)assemble(); break;
    case Action::Run: run(); break;
    case Action::Pause: pause("Paused by user"); refresh(); break;
    case Action::Step: step(); break;
    case Action::Reset: {
      pause("");
      uint8_t input;
      if (get_input(input) && ensure_assembled()) {
        machine_.reset(input);
        execution_timer_.reset();
        breakpoint_stop_ = false;
        set_status("CPU reset / Program reloaded / Breakpoints preserved");
        refresh();
      }
      break;
    }
    case Action::Breakpoint: toggle_breakpoint(); break;
    case Action::ClearBreakpoints: machine_.breakpoints.fill(false); breakpoint_stop_ = false; refresh(); set_status("All breakpoints cleared"); break;
    case Action::Demo:
    case Action::Popcount:
      if (may_replace()) { set_source(action == Action::Demo ? demo_source : popcount_source, "", false); set_status("Example loaded / Save As to keep your own copy"); }
      break;
    case Action::Help: help_.open(); break;
    case Action::Apply: {
      uint8_t input;
      if (get_input(input)) { cpu_set_input_port(&machine_.cpu, input); set_status("Virtual input applied at address 0xEE"); refresh(); }
      break;
    }
    case Action::ClearLog: log_.text(""); break;
    case Action::ApplyClock: apply_clock(); break;
  }
}

int App::smoke_test(const std::string &directory)
{
  // Exercise the same actions as the widgets, without file-dialog interaction.
  show();
  Fl::check();
  if (clock_hz_ != DEFAULT_CLOCK_HZ || execution_timer_.seconds() != 0.0) return 21;
  if (!assemble()) return 10;
  run();
  // Exercise real event-loop pacing as well as the deterministic timer unit tests.
  const auto deadline = ExecutionTimer::Clock::now() + std::chrono::seconds(10);
  while (running_ && ExecutionTimer::Clock::now() < deadline) Fl::wait(TIMER_SECONDS);
  if (cpu_get_output_port(&machine_.cpu) != 4 || machine_.cpu.cycle_count != 126 || running_) return 11;
  const double completed_seconds = execution_timer_.seconds();
  if (execution_timer_.running() || completed_seconds <= 0.0 ||
      text_of(timing_).find("Virtual time: 126.000 us") == std::string::npos) return 22;
  if (!capture(*this, directory + "/studio.png")) return 12;
  clock_input_->value("1000");
  act(Action::ApplyClock);
  if (clock_hz_ != 1000 || machine_.cpu.cycle_count != 126 ||
      execution_timer_.seconds() != completed_seconds ||
      text_of(timing_).find("Virtual time: 126.000 ms") == std::string::npos) return 23;
  clock_input_->value("0");
  act(Action::ApplyClock);
  if (clock_hz_ != 1000 || execution_timer_.seconds() != completed_seconds) return 24;
  clock_input_->value("1000");
  act(Action::Reset);
  if (clock_hz_ != 1000 || machine_.cpu.cycle_count != 0 || execution_timer_.seconds() != 0.0 ||
      execution_timer_.running()) return 25;
  editor_->insert_position(source_.skip_lines(0, static_cast<int>(machine_.image.line_for_address[0] - 1)));
  toggle_breakpoint();
  run();
  Fl::remove_timeout(timer, this);
  tick();
  if (running_ || !breakpoint_stop_ || machine_.cpu.cycle_count != 0) return 13;
  if (execution_timer_.running()) return 26;
  step();
  if (machine_.cpu.cycle_count != 1) return 14;
  if (execution_timer_.running() ||
      text_of(timing_).find("Virtual time: 1.000 ms") == std::string::npos) return 27;
  run();
  pause("Smoke test pause");
  if (running_) return 15;
  const double paused_seconds = execution_timer_.seconds();
  if (execution_timer_.running() || execution_timer_.seconds(
      ExecutionTimer::Clock::now() + std::chrono::hours(1)) != paused_seconds) return 28;
  source_.append("\nLDI A 1\n");
  if (!dirty_ || machine_.assembled || assemble() || error_line_ == 0) return 16;
  if (execution_timer_.seconds() != paused_seconds) return 29;
  if (!capture(*this, directory + "/assembly-error.png")) return 17;
  set_source(demo_source, "", true);
  const std::string save_path = directory + "/smoke-source.asm";
  if (!save_to(save_path) || dirty_ || !read_from(save_path) || text_of(source_) != demo_source || !assemble()) return 18;
  if (execution_timer_.seconds() != 0.0 || clock_hz_ != 1000) return 30;
  set_source("loop:\nJMP loop\n", "", false);
  trace_->value(0);
  speed_->value(2);
  run();
  for (unsigned ticks = 0; running_ && ticks < RUN_INSTRUCTION_LIMIT; ++ticks) { Fl::remove_timeout(timer, this); tick(); }
  if (running_ || machine_.cpu.cycle_count != RUN_INSTRUCTION_LIMIT || machine_.cpu.halted) return 19;
  if (execution_timer_.running()) return 31;
  // Errors stop the real timer too, including invalid opcodes and stack failures.
  for (const auto *invalid_program : {".BYTE 0xFF\n", "POP A\n"}) {
    set_source(invalid_program, "", false);
    run();
    Fl::remove_timeout(timer, this);
    tick();
    if (running_ || execution_timer_.running() || !machine_.cpu.halted ||
        machine_.cpu.cycle_count != 1) return 32;
  }
  set_source(popcount_source, "", false);
  if (execution_timer_.seconds() != 0.0 || machine_.cpu.cycle_count != 0 || clock_hz_ != 1000) return 33;
  help_.open();
  Fl::check();
  if (help_.topic_count() < 28 || !capture(help_, directory + "/help.png")) return 20;
  help_.hide();
  hide();
  std::puts("VM8 Studio GUI: assemble, run, reset, breakpoint, step, pause, edit, error, save, open, limit, clock, timing and help passed.");
  return 0;
}
}
