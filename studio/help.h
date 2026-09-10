#pragma once

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Text_Display.H>
#include <string>
#include <vector>

namespace vm8 {
struct HelpTopic { std::string title; std::string body; };
class HelpWindow : public Fl_Double_Window {
  Fl_Input *search_;
  Fl_Hold_Browser *topics_;
  Fl_Text_Display *display_;
  Fl_Text_Buffer buffer_;
  std::vector<HelpTopic> entries_;
  std::vector<size_t> filtered_;
  void filter();
  void select();
public:
  HelpWindow();
  ~HelpWindow() override;
  void open();
  void resize(int x, int y, int w, int h) override;
  size_t topic_count() const { return entries_.size(); }
};
}
