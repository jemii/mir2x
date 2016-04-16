// generated by Fast Light User Interface Designer (fluid) version 1.0303

#ifndef actionsettestgroup_hpp
#define actionsettestgroup_hpp
#include <FL/Fl.H>
#include <cstdint>
#include <string>
#include "animationset.hpp"
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <cstdint>

class ActionSetTestGroup : public Fl_Group {
public:
  ActionSetTestGroup(int X, int Y, int W, int H, const char *L = 0);
  Fl_Choice *m_TestStatus1;
  Fl_Choice *m_TestDirection1;
  Fl_Choice *m_TestRoundCount1;
  Fl_Choice *m_TestStatus2;
  Fl_Choice *m_TestDirection2;
  Fl_Choice *m_TestRoundCount2;
  Fl_Choice *m_TestStatus3;
  Fl_Choice *m_TestDirection3;
  Fl_Choice *m_TestRoundCount3;
  Fl_Choice *m_TestStatus4;
  Fl_Choice *m_TestDirection4;
  Fl_Choice *m_TestRoundCount4;
  Fl_Choice *m_TestStatus5;
  Fl_Choice *m_TestDirection5;
  Fl_Choice *m_TestRoundCount5;
private:
  inline void cb_Start_i(Fl_Button*, void*);
  static void cb_Start(Fl_Button*, void*);
  inline void cb_Stop_i(Fl_Button*, void*);
  static void cb_Stop(Fl_Button*, void*);
public:
  void AddValidStatus();
  static void StatusItemCallback(Fl_Widget * pWidget, void *pData);
  void AddValidDirection(int nIndex, int nStatus);
  uint32_t GetTestActionSetCode(int nIndex);
  void StartTest();
  void StopTest();
};
#endif