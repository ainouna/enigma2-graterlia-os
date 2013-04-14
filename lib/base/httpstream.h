#ifndef __lib_base_httpstream_h
#define __lib_base_httpstream_h

#include <string>
#include <queue>
#include <time.h>
#include <lib/base/ebase.h>
#include <lib/base/itssource.h>
#include <lib/base/socketbase.h>
#include <lib/base/thread.h>
#include <lib/base/ringbuffer.h>

struct hls_segment {
	std::string url;
	int duration;
};

class eHttpConnection: public Object
{
	int m_fd;
	int m_statuscode;
	std::string m_header;

public:
	int statuscode() { return m_statuscode; }
	int fd() { return m_fd; }
	
	std::string m_request;
// 	bool open
	eHttpConnection();
	~eHttpConnection();
	std::string findHeader(const std::string &name);
	int openUrl(const std::string &url);
	int close();
};

class eHttpStream: public iTsSource, public Object
{
	DECLARE_REF(eHttpStream);

	RingBuffer m_rbuffer;

	std::string m_url;
	int m_streamSocket;

	char*  m_scratch;
	size_t m_scratchSize;
	size_t m_contentLength;
	size_t m_contentServed;
	size_t m_chunkSize;

	bool m_tryToReconnect;
	bool m_chunkedTransfer;
    bool m_firstOffset;
	
	bool m_ishls;
	std::queue<hls_segment> m_segments;
	std::string m_lastSegmentUrl;
	time_t m_nextLoadTime;
	int m_lastMediaSequence;
	
	int openSock(const std::string& url);
	int parsePlaylist(std::string s);
	int readPlaylist(int fd);
	std::string prefixSegment(const std::string &segment);
	
	/* http connection */
// 	std::string findHeader(const std::string name);

	/* iTsSource */
	ssize_t read(off_t offset, void *buf, size_t count);
	off_t length();
	off_t offset();
	int valid();
	int close();

public:
	eHttpStream();
	~eHttpStream();
	int open(const std::string& url);
};

#endif
