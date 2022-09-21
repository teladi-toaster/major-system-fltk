#pragma once

#include <fstream>
#include <iterator>
#include <iostream>
#include <regex>
#include <sstream>
#include <vector>


using namespace std;

void set_digits(string* number, uint* digit1, uint* digit2, uint* digit3) {
  const char* s = number->c_str();
  uint* digits[3] = {digit1, digit2, digit3};
  for(int i=0; i<number->size(); i++) {
    int digit_num = i + 3-number->size();
    *digits[digit_num] = s[i] - 0x30;
  }
}

struct DictionaryWord {
  string word;
  string ipa;
};

enum PhonemeTripleMode { CCC, CVC };

// See https://en.wikipedia.org/wiki/English_phonology
// TODO: Consider basing entire model on these tables for ease of use
const vector<string> VOWELS{
  "æ",  /*trAp*/
  "ɑː", /*bAth, pAlm*/
  "ɒ",  /*lOt, clOth*/
  "ɔː", /*thOUGHt*/
  "ɪ",  /*kIt*/
  "e",  /*drEss*/ // ɛ
  "ʌ",  /*strUt*/
  "ʊ",  /*fOOt, pUss*/
  
  "eɪ", /*fAce*/
  "əʊ", /*gOAt*/
  "iː", /*flEEce*/
  "uː", /*gOOse*/
  
  "aɪ", /*prIce*/
  "ɔɪ", /*chOIce*/
  "aʊ", /*mOUth*/
  
  "ɜː", /*nURse*/
  "ɑː", /*stARt*/
  "ɔː", /*nORth, fORce*/
  "ɪə", /*nEAr*/
  "ɛː", /*squARE*/
  "ʊə", /*cURE*/
  
  "ə",  /*commA, lettER*/
  "i"   /*happY*/
};

const vector<string> CONSONANTS{
"p", /*Pit*/    "b",  /*Bit*/
"t", /*Tin*/ 	  "d",  /*Din*/
"k", /*Cut*/ 	  "ɡ",  /*Gut*/
"t", /*CHeap*/ 	"dʒ", /*Jeep*/
"f", /*Fat*/ 	  "v",  /*Vat*/
"θ", /*THigh*/ 	"ð",  /*THy*/
"s", /*Sap*/ 	  "z",  /*Zap*/
"ʃ", /*meSHer*/ "ʒ",  /*meaSure*/
"x", /*loCH*/
"h", /*Ham*/
"m", /*tiM*/
"n", /*tiN*/
"ŋ", /*tiNG*/
"j", /*Your*/
"w", /*Wore*/
"r", /*Rump*/
"l", /*Lump*/
};

string phonemes_to_group(const vector<string> *phonemes) {
  ostringstream ss;
  ss << "(";
  uint total_phonemes = phonemes->size();
  for(uint i=0; i<total_phonemes; i++) {
    ss << (*phonemes)[i];
    if(i < total_phonemes-1) {
      ss << "|";
    }
  }
  ss << ")";
  return ss.str();
}

class PhonemePatternSettings {
  public:
    PhonemeTripleMode triple_mode = CVC;
    bool strict_mode = false;

    vector<string> digit0_c{"s", "z"};
    vector<string> digit1_c{"t", "d"};
    vector<string> digit2_c{"n"};
    vector<string> digit3_c{"m"};
    vector<string> digit4_c{"r", "ɹ"}; // ɹ
    vector<string> digit5_c{"l"};
    vector<string> digit6_c{"tʃ", "ʃ", "dʒ", "ʒ"};
    vector<string> digit7_c{"k", "ɡ", "kw"};
    vector<string> digit8_c{"f", "v"};
    vector<string> digit9_c{"p", "b"};

    vector<string> digit0_v{"uː"};
    vector<string> digit1_v{"æ"};
    vector<string> digit2_v{"e", "ɛ"};
    vector<string> digit3_v{"ɪ"};
    vector<string> digit4_v{"ɒ"};
    vector<string> digit5_v{"ʊ"};
    vector<string> digit6_v{"eɪ"};
    vector<string> digit7_v{"iː"};
    vector<string> digit8_v{"aɪ"};
    vector<string> digit9_v{"əʊ"};
  
    string any_vowel;
    string ignored = "[ˈˌ]"; // "ː" ignored only if rhotic
    string rhotic = "[ː]";
  
    PhonemePatternSettings() {
      any_vowel = phonemes_to_group(&VOWELS);
    }
    
    vector<string> get_consonants(uint8_t i) {
      switch(i) {
        case 0:
          return digit0_c;
          break;
        case 1:
          return digit1_c;
          break;
        case 2:
          return digit2_c;
          break;
        case 3:
          return digit3_c;
          break;
        case 4:
          return digit4_c;
          break;
        case 5:
          return digit5_c;
          break;
        case 6:
          return digit6_c;
          break;
        case 7:
          return digit7_c;
          break;
        case 8:
          return digit8_c;
          break;
        case 9:
          return digit9_c;
          break;
        default:
          // should throw exception but don't know them yet
          return digit0_c;
      }
    }

    vector<string> get_vowels(uint8_t i) {
      switch(i) {
        case 0:
          return digit0_v;
          break;
        case 1:
          return digit1_v;
          break;
        case 2:
          return digit2_v;
          break;
        case 3:
          return digit3_v;
          break;
        case 4:
          return digit4_v;
          break;
        case 5:
          return digit5_v;
          break;
        case 6:
          return digit6_v;
          break;
        case 7:
          return digit7_v;
          break;
        case 8:
          return digit8_v;
          break;
        case 9:
          return digit9_v;
          break;
        default:
          // should throw exception but don't know them yet
          return digit0_v;
      }
    }
};

class DigitPatternCompiler {
  public:
    PhonemePatternSettings* settings;
  
    DigitPatternCompiler(PhonemePatternSettings* settings) {
      this->settings = settings;
    }
  
    string compile_pattern(uint8_t digit1, uint8_t digit2, uint8_t digit3) {
      vector<string> phoneme1 = settings->get_consonants(digit1);
      vector<string> phoneme2;
      vector<string> phoneme3 = settings->get_consonants(digit3);
      if(settings->triple_mode == CCC) {
        phoneme2 = settings->get_consonants(digit2);
      } else {
        phoneme2 = settings->get_vowels(digit2);
      }
      ostringstream ss;
      ss << phonemes_to_group(&phoneme1);
      ss << settings->ignored << "*";
      if(settings->triple_mode == CCC) {
        ss << settings->any_vowel << "*" << settings->rhotic << "?";
        ss << settings->ignored << "*";
      }
      ss << phonemes_to_group(&phoneme2);
      ss << settings->ignored << "*";
      if(settings->triple_mode == CCC) {
        ss << settings->any_vowel << "*" << settings->rhotic << "?";
        ss << settings->ignored << "*";
      }
      ss << phonemes_to_group(&phoneme3);
      if(!settings->strict_mode) {
        ss << ".*";
      }

      cout << "Pattern for " << +digit1 << +digit2 << +digit3;
      cout << "'" << phoneme1[0] << phoneme2[0] << phoneme3[0] << "'";
      cout << ": /" << ss.str() << endl;
      return ss.str();
    }
};

class WordContainer {
  public:
    vector<DictionaryWord> words;
    vector<uint> match_indexes;
  
    WordContainer(const string* data_str) {
      stringstream ss(*data_str);
      string line;
      smatch m;
      regex line_template = regex("([^\t]+)\t/([^/]+)/", regex::extended);
      while(getline(ss, line)) {
        if(!regex_search(line, m, line_template)) { break; }
        words.push_back(DictionaryWord{m[1], m[2]});
      }
    }

    // TODO: Add file loader gui or remove this
    WordContainer(string file_name) {
      fstream file;
      file.open(file_name, ios::in);
      if (file.is_open()) {
        string line;
        smatch m;
        regex line_template = regex("([^\t]+)\t/([^/]+)/", regex::extended);
        while(getline(file, line)) {
          if(!regex_search(line, m, line_template)) { break; }
          words.push_back(DictionaryWord{m[1], m[2]});
        }
      } else {
        cout << "Could not open file " << file_name << endl;
      }
    }

    uint set_filter(regex pattern) {
      match_indexes.clear();
      for(uint i=0; i<words.size(); i++) {
        if(regex_match(words[i].ipa, pattern)) {
          match_indexes.push_back(i);
        }
      }
    
      return match_indexes.size();
    }
};
