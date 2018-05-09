/*
 * server.cpp : Live555 interface for server side: streaming to udp sockets directly or by using an on-demand rtsp server
 * 
 * Copyright 2017, 2018 Valkka Security Ltd. and Sampsa Riikonen.
 * 
 * Authors: Sampsa Riikonen <sampsa.riikonen@iki.fi>
 * 
 * This file is part of the Valkka library.
 * 
 * Valkka is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

/** 
 *  @file    server.cpp
 *  @author  Sampsa Riikonen
 *  @date    2017
 *  @version 0.4.0 
 *  
 *  @brief   Live555 interface for server side: streaming to udp sockets directly or by using an on-demand rtsp server
 */ 

#include "tools.h"
#include "liveserver.h"


BufferSource::BufferSource(UsageEnvironment& env, FrameFifo &fifo, unsigned preferredFrameSize, unsigned playTimePerFrame, unsigned offset) : FramedSource(env), fifo(fifo), fPreferredFrameSize(preferredFrameSize), fPlayTimePerFrame(playTimePerFrame), offset(offset), active(true) {
}

BufferSource::~BufferSource() {
  for(auto it=internal_fifo.begin(); it!=internal_fifo.end(); ++it) {
    fifo.recycle(*it);
  }
}


void BufferSource::handleFrame(Frame* f) {
#ifdef STREAM_SEND_DEBUG
  std::cout << "BufferSource : handleFrame" << std::endl;
#endif
  
  if (f->getFrameClass()!=FrameClass::basic) { // just accept BasicFrame's
    fifo.recycle(f);
    return;
  }
  
  internal_fifo.push_front(static_cast<BasicFrame*>(f));
  if (!active) { // time to activate and re-schedule doGetNextFrame
#ifdef STREAM_SEND_DEBUG
    std::cout << "BufferSource : evoking doGetNextFrame" << std::endl;
#endif
    doGetNextFrame();
  }
}


void BufferSource::doGetNextFrame() {
  /* // testing ..
  std::cout << "doGetNextFrame!\n";
  return;
  */
  
  /*
  http://lists.live555.com/pipermail/live-devel/2014-October/018768.html
  
  control flow:
  http://www.live555.com/liveMedia/faq.html#control-flow
  
  use of discrete framer:
  http://lists.live555.com/pipermail/live-devel/2014-September/018686.html
  http://lists.live555.com/pipermail/live-devel/2011-November/014019.html
  http://lists.live555.com/pipermail/live-devel/2016-January/019856.html
  
  this::doGetNextFrame => FramedSource::afterGetting(this) [resets fIsCurrentlyAwaitingData] => calls "AfterGettingFunc" callback (defined god-knows-where) => .. end up calling 
  FramedSource::getNextFrame [this tilts if fIsCurrentlyAwaitingData is set]
  http://lists.live555.com/pipermail/live-devel/2005-August/002995.html
  
  */
  
  /* // insight from "FramedSource.hh":
  // The following variables are typically accessed/set by doGetNextFrame()
  unsigned char* fTo; // in
  unsigned fMaxSize; // in
  
  unsigned fFrameSize; // out
  unsigned fNumTruncatedBytes; // out
  struct timeval fPresentationTime; // out
  unsigned fDurationInMicroseconds; // out
  */
  BasicFrame* f;
  unsigned fMaxSize_;
  
#ifdef STREAM_SEND_DEBUG
  std::cout << "BufferSource : doGetNextFrame : " << std::endl;
#endif
  
  if (internal_fifo.empty()) {
#ifdef STREAM_SEND_DEBUG
    std::cout << "BufferSource : doGetNextFrame : fifo empty (1) " << std::endl;
#endif
    active=false; // this method is not re-scheduled anymore .. must be called again.
    return;
  }
  active=true; // this will be re-scheduled

  f=internal_fifo.back(); // take the last element
  
  fFrameSize =(unsigned)f->payload.size()-offset;
  
  if (fMaxSize>=fFrameSize) { // unsigned numbers ..
    fNumTruncatedBytes=0;
  }
  else {
    fNumTruncatedBytes =fFrameSize-fMaxSize;
    // fNumTruncatedBytes+=10; // stupid debugging
  }
  
#ifdef STREAM_SEND_DEBUG
  std::cout << "BufferSource : doGetNextFrame : frame        " << *f << std::endl;
  std::cout << "BufferSource : doGetNextFrame : fMaxSize     " << fMaxSize << std::endl;
  std::cout << "BufferSource : doGetNextFrame : payload size " << f->payload.size() << std::endl;
  std::cout << "BufferSource : doGetNextFrame : fFrameSize   " << fFrameSize << std::endl;
  std::cout << "BufferSource : doGetNextFrame : fNumTruncB   " << fNumTruncatedBytes << std::endl;
  std::cout << "BufferSource : doGetNextFrame : payload      " << f->dumpPayload() << std::endl;
#endif
  
  fFrameSize=fFrameSize-fNumTruncatedBytes;
  
  // memcpy(fTo, f->payload.data(), f->payload.size());
  memcpy(fTo, f->payload.data()+offset, fFrameSize);
  // fMaxSize  =f->payload.size();
  // fNumTruncatedBytes=0;
  
  fPresentationTime=msToTimeval(f->mstimestamp); // timestamp to time struct
  
  // fDurationInMicroseconds = 0;
  // fPresentationTime.tv_sec   =(fMstimestamp/1000); // secs
  // fPresentationTime.tv_usec  =(fMstimestamp-fPresentationTime.tv_sec*1000)*1000; // microsecs
  // std::cout << "call_afterGetting: " << fPresentationTime.tv_sec << "." << fPresentationTime.tv_usec << " " << fFrameSize << "\n";
  
#ifdef STREAM_SEND_DEBUG
  std::cout << "BufferSource : doGetNextFrame : recycle     " << *f << std::endl;
#endif
  fifo.recycle(f); // return the frame to the main live555 incoming fifo
#ifdef STREAM_SEND_DEBUG
  fifo.diagnosis();
#endif
  internal_fifo.pop_back();
  
  /*
  if (fNumTruncatedBytes>0) {
    active=false;
    return;
  }
  */
  
  if (internal_fifo.empty()) {
#ifdef STREAM_SEND_DEBUG
    std::cout << "BufferSource : doGetNextFrame : fifo will be empty " << std::endl;
#endif
    fDurationInMicroseconds = 0;
  }
  else { // approximate when this will be called again
#ifdef STREAM_SEND_DEBUG
    std::cout << "BufferSource : doGetNextFrame : more frames in fifo " << std::endl;
#endif
    fDurationInMicroseconds = 0; // call again immediately
  }
  
#ifdef STREAM_SEND_DEBUG
  std::cout << "BufferSource : doGetNextFrame : calling afterGetting\n";
#endif
  FramedSource::afterGetting(this); // will re-schedule BufferSource::doGetNextFrame()
}


Stream::Stream(UsageEnvironment &env, FrameFifo &fifo, const std::string adr, unsigned short int portnum, const unsigned char ttl) : env(env), fifo(fifo) {
  /*
  UsageEnvironment& env;
  RTPSink*          sink;
  RTCPInstance*     rtcp;
  FramedSource*     source;
  Groupsock* rtpGroupsock;
  Groupsock* rtcpGroupsock;
  unsigned char cname[101];

  BufferSource* buffer_source;
  */
  
  // Create 'groupsocks' for RTP and RTCP:
  struct in_addr destinationAddress;
  //destinationAddress.s_addr = chooseRandomIPv4SSMAddress(*env);
  destinationAddress.s_addr=our_inet_addr(adr.c_str());

  Port rtpPort(portnum);
  Port rtcpPort(portnum+1);

  // Groupsock rtpGroupsock(this->env, destinationAddress, rtpPort, ttl);
  this->rtpGroupsock  =new Groupsock(this->env, destinationAddress, rtpPort, ttl);
  // this->rtpGroupsock->multicastSendOnly();
  
  int fd=this->rtpGroupsock->socketNum();
  // increaseSendBufferTo(this->env,fd,this->nsocket); // TODO
  
  this->rtcpGroupsock =new Groupsock(this->env, destinationAddress, rtcpPort, ttl);
  // this->rtcpGroupsock->multicastSendOnly();
  // rtpGroupsock.multicastSendOnly(); // we're a SSM source
  // Groupsock rtcpGroupsock(*env, destinationAddress, rtcpPort, ttl);
  // rtcpGroupsock.multicastSendOnly(); // we're a SSM source

  // Create a 'H264 Video RTP' sink from the RTP 'groupsock':
  // OutPacketBuffer::maxSize = 100000;
  
  unsigned char CNAME[101];
  gethostname((char*)CNAME, 100);
  CNAME[100] = '\0'; // just in case ..
  memcpy(this->cname,CNAME,101);
}


void Stream::handleFrame(Frame* f) {
  buffer_source->handleFrame(f); // buffer source recycles the frame when ready
}

void Stream::startPlaying() {
  sink->startPlaying(*(terminal), this->afterPlaying, this);
}

void Stream::afterPlaying(void *cdata) {
  Stream* stream=(Stream*)cdata;
  stream->sink->stopPlaying();
  // Medium::close(stream->buffer_source);
}


Stream::~Stream() {
  Medium::close(buffer_source);
  delete rtpGroupsock;
  delete rtcpGroupsock;
  delete buffer_source;
}



ValkkaServerMediaSubsession::ValkkaServerMediaSubsession(UsageEnvironment& env, FrameFifo &fifo, Boolean reuseFirstSource) : OnDemandServerMediaSubsession(env, reuseFirstSource), fifo(fifo) {
}


ValkkaServerMediaSubsession::~ValkkaServerMediaSubsession() {
}


void ValkkaServerMediaSubsession::handleFrame(Frame *f) {
  buffer_source->handleFrame(f); // buffer source recycles the frame when ready
}


  
H264Stream::H264Stream(UsageEnvironment &env, FrameFifo &fifo, const std::string adr, unsigned short int portnum, const unsigned char ttl) : Stream(env,fifo,adr,portnum,ttl) {
  buffer_source  =new BufferSource(env, fifo, 0, 0, 4); // nalstamp offset: 4  
  // OutPacketBuffer::maxSize = this->npacket; // TODO
  // http://lists.live555.com/pipermail/live-devel/2013-April/016816.html
  sink = H264VideoRTPSink::createNew(env,rtpGroupsock, 96);
  // this->rtcp      = RTCPInstance::createNew(this->env, this->rtcpGroupsock, 500,  this->cname, sink, NULL, True); // saturates the event loop!
  terminal       =H264VideoStreamDiscreteFramer::createNew(env, buffer_source);
}


H264Stream::~H264Stream() {
  // delete sink;
  // delete terminal;
  Medium::close(sink);
  Medium::close(terminal);
}



H264ServerMediaSubsession* H264ServerMediaSubsession::createNew(UsageEnvironment& env, FrameFifo &fifo, Boolean reuseFirstSource) {
  return new H264ServerMediaSubsession(env, fifo, reuseFirstSource);
}


H264ServerMediaSubsession::H264ServerMediaSubsession(UsageEnvironment& env, FrameFifo &fifo, Boolean reuseFirstSource) : ValkkaServerMediaSubsession(env, fifo, reuseFirstSource),
  fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL) {
}


H264ServerMediaSubsession::~H264ServerMediaSubsession() {
  delete[] fAuxSDPLine;
}


static void afterPlayingDummy(void* clientData) {
  H264ServerMediaSubsession* subsess = (H264ServerMediaSubsession*)clientData;
  subsess->afterPlayingDummy1();
}


void H264ServerMediaSubsession::afterPlayingDummy1() {
  // Unschedule any pending 'checking' task:
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  // Signal the event loop that we're done:
  setDoneFlag();
}


static void checkForAuxSDPLine(void* clientData) {
  H264ServerMediaSubsession* subsess = (H264ServerMediaSubsession*)clientData;
  subsess->checkForAuxSDPLine1();
}


void H264ServerMediaSubsession::checkForAuxSDPLine1() {
  nextTask() = NULL;

  char const* dasl;
  if (fAuxSDPLine != NULL) {
    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL) {
    fAuxSDPLine = strDup(dasl);
    fDummyRTPSink = NULL;

    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (!fDoneFlag) {
    // try again after a brief delay:
    int uSecsToDelay = 100000; // 100 ms
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)checkForAuxSDPLine, this);
  }
}


char const* H264ServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
  if (fAuxSDPLine != NULL) return fAuxSDPLine; // it's already been set up (for a previous client)

  if (fDummyRTPSink == NULL) { // we're not already setting it up for another, concurrent stream
    // Note: For H264 video files, the 'config' information ("profile-level-id" and "sprop-parameter-sets") isn't known
    // until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
    // and we need to start reading data from our file until this changes.
    fDummyRTPSink = rtpSink;

    // Start reading the file:
    fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

    // Check whether the sink's 'auxSDPLine()' is ready:
    checkForAuxSDPLine(this);
  }

  envir().taskScheduler().doEventLoop(&fDoneFlag);

  return fAuxSDPLine;
}


FramedSource* H264ServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 500; // kbps, estimate

  // Create the video source:
  buffer_source  =new BufferSource(envir(), fifo, 0, 0, 4); // nalstamp offset: 4
  
  // Create a framer for the Video Elementary Stream:
  return H264VideoStreamFramer::createNew(envir(), buffer_source);
}


RTPSink* H264ServerMediaSubsession ::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* /*inputSource*/) {
  return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
