#include <FL/Enumerations.H>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Table.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>

#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <vector>

#include "phonetics.h"
#include "wordlists.h"


// https://www.fltk.org/doc-1.3/common.html
//
// https://docs.microsoft.com/en-us/cpp/cpp/constructors-cpp?view=msvc-170
//
// https://stackoverflow.com/questions/64779163/sum-types-in-c#answer-64779931
// https://en.cppreference.com/w/cpp/utility/variant
//
// https://www.geeksforgeeks.org/vector-in-cpp-stl/
//
// https://github.com/fltk/fltk/blob/master/examples/table-simple.cxx

using namespace std;
// use std::variant

const regex ISNUM_REGEX = regex("[0-9]{1,3}", regex::extended);
const int WIN_WIDTH = 340;
const int WIN_HEIGHT = 600;

class WordTable : public Fl_Table {
  private:
    WordContainer *word_container;

    void event_callback() {
      int r = callback_row();
      int c = callback_col();
      TableContext context = callback_context();
      fprintf(stderr, "callback: Row=%d Col=%d Context=%d Event=%d\n",
              r, c, (int)context, (int)Fl::event());
    }

    static void event_callback_wrapper(Fl_Widget* o, void* data) {
      WordTable* word_table = (WordTable*)o;
      //MainGui* main_gui = (MainGui*)data;
      word_table->event_callback();
    }

    void DrawHeader(const char *s, int x, int y, int w, int h) {
      fl_push_clip(x,y,w,h);
        fl_draw_box(FL_THIN_UP_BOX, x,y,w,h, row_header_color());
        fl_color(FL_BLACK);
        fl_draw(s, x,y,w,h, FL_ALIGN_CENTER);
      fl_pop_clip();
    }

    void DrawData(const char *s, int x, int y, int w, int h) {
      fl_push_clip(x,y,w,h);
        fl_color(FL_WHITE); fl_rectf(x,y,w,h);
        fl_color(FL_GRAY0); fl_draw(s, x,y,w,h, FL_ALIGN_CENTER);
        fl_color(color()); fl_rect(x,y,w,h);
      fl_pop_clip();
    }

    void draw_cell(TableContext context, int row=0, int col=0, int x=0, int y=0, int w=0, int h=0) {
      switch(context) {
        case CONTEXT_STARTPAGE:

          return;
        case CONTEXT_COL_HEADER:
          DrawHeader(col==0 ? "Word" : "IPA", x,y,w,h);
          return;
        case CONTEXT_CELL:
          {
            // DictionaryWord* word = &word_container->words[row];
            //if(word_container->match_indexes.size() > 0) {
            if(valid_input) {
              // cout << "Row " << row << ", match_indexes " << word_container->match_indexes.size() << endl;
              uint index = word_container->match_indexes[row];
              DictionaryWord* word = &word_container->words[index];
              DrawData(col==0 ? word->word.c_str() : word->ipa.c_str(), x,y,w,h);
            } else { 
              DictionaryWord* word = &word_container->words[row];
              DrawData(col==0 ? word->word.c_str() : word->ipa.c_str(), x,y,w,h);
            }
          }
          return;
        default:
          return;
      }
    }

  public:
    bool valid_input = False;
    WordTable(int x, int y, int w, int h, WordContainer* word_container, const char *L=0) : Fl_Table(x,y,w,h,L) {
      this->word_container = word_container;
      // rows
      rows(word_container->words.size());
      row_header(0);

      // cols
      cols(2);
      col_header(1);
      col_resize(1);
      col_width_all(160);

      table->callback(&event_callback_wrapper, (void*)this);
      table->when(FL_WHEN_CHANGED);
      end();
    }
    ~WordTable() { }
};

class MainGui {
  private:
    Fl_Window* window;
    Fl_Input* input;
    Fl_Check_Button* switch_ccc_cvc;
    Fl_Check_Button* switch_strict;
    Fl_Box* pattern_label;
    Fl_Box* error_box;
    WordContainer* word_container;
    WordTable* word_table;
    PhonemePatternSettings* pattern_settings;
    DigitPatternCompiler* pattern_compiler;
    string current_pattern = "";

    void on_input_update(Fl_Input* input) {
      cout << "Value: " << input->value() << endl;
      smatch match;
      string s = input->value();
      bool result = regex_match(s, match, ISNUM_REGEX);
      if (result) {
        error_box->hide();
        uint digit1 = 0;
        uint digit2 = 0;
        uint digit3 = 0;
        cout << "Digits pre:  " << digit1 << digit2 << digit3 << endl;
        set_digits(&s, &digit1, &digit2, &digit3);
        cout << "Digits post: " << digit1 << digit2 << digit3 << endl;
        string pattern_str = pattern_compiler->compile_pattern(digit1, digit2, digit3);
        current_pattern = pattern_str;
        pattern_label->label(current_pattern.c_str());
        regex pattern = regex(pattern_str, regex::extended);
        uint num_matches = word_container->set_filter(pattern);
        word_table->rows(num_matches);
        word_table->valid_input = True;
        word_table->redraw();
      } else {
        error_box->show();
        pattern_label->label("");
        word_table->rows(word_container->words.size());
        word_table->valid_input = False;
        word_table->redraw();
      }
    }

    static void on_input_update_callback(Fl_Widget* o, void* f) {
      Fl_Input* input = (Fl_Input*)o;
      ((MainGui*)f)->on_input_update(input);
    }
  
    void on_ccc_cvc_changed(Fl_Check_Button* button) {
      pattern_settings->triple_mode = button->value() == 0 ? CCC : CVC;
      cout << "ccc_cvc changed " << button->value() << endl;
      on_input_update(input);
    }
  
    static void on_ccc_cvc_callback_wrapper(Fl_Widget* o, void* f) {
      Fl_Check_Button* button = (Fl_Check_Button*)o;
      ((MainGui*)f)->on_ccc_cvc_changed(button);
    }

    void on_strict_changed(Fl_Check_Button* button) {
      // inverted logic because button says "incomplete match" aka ticked is unstrict
      pattern_settings->strict_mode = button->value() == 0 ? True : False;
      cout << "strict changed " << button->value() << endl;
      on_input_update(input);
    }
  
    static void on_strict_callback_wrapper(Fl_Widget* o, void* f) {
      Fl_Check_Button* button= (Fl_Check_Button*)o;
      ((MainGui*)f)->on_strict_changed(button);
    }

  public:
    MainGui() {
      this->window = new Fl_Window(WIN_WIDTH, WIN_HEIGHT);

      this->switch_ccc_cvc= new Fl_Check_Button(0, 0, 40, 20, "Second digit vowel");
      this->switch_ccc_cvc->value(1);
      this->switch_ccc_cvc->callback(on_ccc_cvc_callback_wrapper, (void*)this);

      this->switch_strict= new Fl_Check_Button(WIN_WIDTH/2, 0, 40, 20, "Incomplete match");
      this->switch_strict->value(1);
      this->switch_strict->callback(on_strict_callback_wrapper, (void*)this);

      this->input = new Fl_Input(WIN_WIDTH/2-20, 30, 40, 20, "Number");
      input->callback(on_input_update_callback, (void*)this);
      input->when(FL_WHEN_CHANGED);

      this->pattern_label = new Fl_Box(0, 60, WIN_WIDTH, 20, "");
      pattern_label->box(FL_NO_BOX);

      this->error_box = new Fl_Box(0, 80, WIN_WIDTH, 20, "Input must be number 0-999");
      error_box->box(FL_NO_BOX);
      error_box->labelcolor(FL_RED);

      WordContainer* word_container = new WordContainer(&defaults::english_ipa_str);
      this->word_container = word_container;
      this->word_table = new WordTable(0, 100, WIN_WIDTH, WIN_HEIGHT-100, word_container);

      this->pattern_settings = new PhonemePatternSettings();
      this->pattern_compiler = new DigitPatternCompiler(pattern_settings);
      cout << "Loaded " << this->word_container->words.size() << " words" << endl;
      cout << "Vowels: '" << pattern_settings->any_vowel << "'" << endl;
      //cout << "Pattern: '" << pattern_compiler->compile_pattern(1, 2, 3) << "'" << endl;
      //cout << "First word " << word_container->get_word(0)->word << ", " << word_container->get_word(0)->ipa << endl;

      window->end();
      window->resizable(this->word_table);
    }

    int run() {
      this->window->show();
      return Fl::run();
    }
};

int main(int argc, char **argv) {
  MainGui *main_gui = new MainGui();
  return main_gui->run();
}

// TODO: Create digit -> regex compiler
// Both CCC and CVC methods, possibly third AAA where A=C or V
// English sound classes - simplified IPA regex groups that may be combined
// e.g. (sh="ʃ", j_soft="dʒ", ch_hard="tʃ")
//   to make rulegroups like 7=(k, hard_c, hard_g, hard_ch, q, qu)
//                      into   /(k|ɡ|tʃ|kw)/ referred to here as e.g. [:hard:]
//   such that they may be combined for full regexes
//   e.g. /[:hard:]([:ignored:]|[:vowels:])*[:soft:]([:ignored:][:vowels:]*)[:td:]/
//        to represent 761 under a major system using only consonants
//   or   /[:hard:][:ignored:]*[:ay:][:ignored:]*[:td:]/ for ben style with vowels
//   the [:ignored:] class here represents universally ignored things like
//   pauses <,> and glottal stops <'> as well as system-specific ignores 
//   like "h", "w", or "y" depending upon settings
// Note:
//  Some systems use 6=ch/sh/j/zh, 7=k/g
//    others may use 6=sh/j,       7=/k/g/ch
//  Remember optional rhotic after vowels
