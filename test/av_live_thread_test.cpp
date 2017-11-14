/*
 * av_live_thread_test.cpp : Test producer (live thread) consumer (av thread)
 * 
 * Copyright 2017 Valkka Security Ltd. and Sampsa Riikonen.
 * 
 * Authors: Sampsa Riikonen <sampsa.riikonen@iki.fi>
 * 
 * This file is part of Valkka library.
 * 
 * Valkka is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Valkka is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with Valkka.  If not, see <http://www.gnu.org/licenses/>. 
 * 
 */

/** 
 *  @file    av_live_thread_test.cpp
 *  @author  Sampsa Riikonen
 *  @date    2017
 *  @version 0.1
 *  
 *  @brief Test producer (live thread) consumer (av thread)
 *
 */ 

#include "queues.h"
#include "livethread.h"
#include "avthread.h"
#include "filters.h"
#include "logging.h"

using namespace std::chrono_literals;
using std::this_thread::sleep_for;  // http://en.cppreference.com/w/cpp/thread/sleep_for

const char* stream_1   =std::getenv("VALKKA_TEST_RTSP_1");
const char* stream_2   =std::getenv("VALKKA_TEST_RTSP_2");
const char* stream_sdp =std::getenv("VALKKA_TEST_SDP");


void test_1() {
  const char* name = "@TEST: av_live_thread_test: test 1: ";
  std::cout << name <<"** @@Send frames from live to av thread **" << std::endl;
  
  if (!stream_1) {
    std::cout << name <<"ERROR: missing test stream 1: set environment variable VALKKA_TEST_RTSP_1"<< std::endl;
    exit(2);
  }
  std::cout << name <<"** test rtsp stream 1: "<< stream_1 << std::endl;
  
  // *************
  // filtergraph:
  // (LiveThread:livethread) --> {FrameFilter:info} --> {FifoFrameFilter:terminal} --> [FrameFifo:in_fifo] -->> (AVThread:avthread) --> {InfoFrameFilter:decoded_info}
  
  InfoFrameFilter decoded_info("decoded_info");

  FrameFifo       in_fifo     ("in_fifo",10);                
  AVThread        avthread    ("avthread",in_fifo,decoded_info);
  
  FifoFrameFilter terminal    ("terminal",in_fifo); 
  InfoFrameFilter info        ("info",&terminal);       
  LiveThread      livethread  ("livethread"); 
  // *************
  
  LiveConnectionContext ctx;
  bool verbose;
  
  std::cout << name << "starting threads" << std::endl;
  livethread.startCall();
  avthread.  startCall();

  avthread.decodingOnCall();
  
  sleep_for(2s);
  
  std::cout << name << "registering stream" << std::endl;
  ctx = (LiveConnectionContext){LiveConnectionType::rtsp, std::string(stream_1), 2, &info}; // Request livethread to write into filter info
  livethread.registerStreamCall(ctx);
  
  sleep_for(1s);
  
  std::cout << name << "playing stream !" << std::endl;
  livethread.playStreamCall(ctx);
  
  sleep_for(10s);
  
  std::cout << name << "stopping threads" << std::endl;
  livethread.stopCall();
  avthread.  stopCall();
  
}


void test_2() {
}


void test_3() {
}


void test_4() {
}


void test_5() {
}


int main(int argc, char** argcv) {
  if (argc<2) {
    std::cout << argcv[0] << " needs an integer argument.  Second interger argument (optional) is verbosity" << std::endl;
  }
  else {
    
    if  (argc>2) { // choose verbosity
      switch (atoi(argcv[2])) {
        case(0): // shut up
          ffmpeg_av_log_set_level(0);
          fatal_log_all();
          break;
        case(1): // normal
          break;
        case(2): // more verbose
          ffmpeg_av_log_set_level(100);
          debug_log_all();
          break;
        case(3): // extremely verbose
          ffmpeg_av_log_set_level(100);
          crazy_log_all();
          break;
        default:
          std::cout << "Unknown verbosity level "<< atoi(argcv[2]) <<std::endl;
          exit(1);
          break;
      }
    }
    
    switch (atoi(argcv[1])) { // choose test
      case(1):
        test_1();
        break;
      case(2):
        test_2();
        break;
      case(3):
        test_3();
        break;
      case(4):
        test_4();
        break;
      case(5):
        test_5();
        break;
      default:
        std::cout << "No such test "<<argcv[1]<<" for "<<argcv[0]<<std::endl;
    }
  }
} 



