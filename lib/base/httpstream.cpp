#include <cstdio>
#include <openssl/evp.h>

#include <sstream>

#include <lib/base/httpstream.h>
#include <lib/base/eerror.h>


#define MIN(a,b) ((a) < (b) ? (a) : (b))

const long RBUFFER_SIZE = 1024*1024*3;

eHttpConnection::eHttpConnection() :
	 m_fd(-1)
	,m_statuscode(-1)
	,m_header("")
	,m_request("")
{

}

eHttpConnection::~eHttpConnection()
{
//	close();
}


DEFINE_REF(eHttpStream);

eHttpStream::eHttpStream() : 
	 m_rbuffer(RBUFFER_SIZE)
	,m_url("")
	,m_streamSocket(-1)
	,m_m3uSocket(-1)
	,m_scratch(NULL)
	,m_scratchSize(0)
	,m_contentLength(0)
	,m_contentServed(0)
	,m_chunkSize(0)
	,m_tryToReconnect(false)
	,m_chunkedTransfer(false)
	,m_firstOffset(true)
	,m_nextLoadTime(0)
	,m_lastMediaSequence(-1)
	,m_lastSegmentUrl("")
{
}

eHttpStream::~eHttpStream()
{
	close();
}

int eHttpStream::open(const std::string& url)
{
	m_url = url;
	return 0;
}

// bool caseinsesitive_compare (const std::string& left, const std::string& right) {
// 	return std::toupper(left) == std::toupper(right);
// }

std::string eHttpConnection::findHeader(const std::string& name)
{
	std::string val("");
	size_t pos = 0;
	
// 	eDebug("search for header %s", name.c_str());
	
	while (pos != std::string::npos) {
// 		eDebug("pos %d ", pos);
		size_t i;
		for (i = 0; i < name.size() && pos+i < m_header.size(); i++ ) {
			if (tolower(name[i]) != tolower(m_header[pos+i]) ) { break; }
		}
		if (i == name.size()) {
			size_t endpos = m_header.find("\r\n", pos);
// 			eDebug("endpos %d ", endpos);
			if ( endpos != std::string::npos ) {
				val = m_header.substr(pos+i, endpos - pos - i);
// 				eDebug("found header %s", val.c_str());
				return val;
			}
		}
		pos = m_header.find("\r\n", pos);
		if (pos == std::string::npos) { break; }
		pos += 2;
	}
	return val;
}

int eHttpStream::openSock(const std::string& url)
{
	eDebug("eHttpStream::%s", __FUNCTION__);
	
	std::string currenturl(url);
	
// 	if (!m_tryToReconnect) {
// 		m_contentServed = 0;
// 		request.append("Range: bytes=0-\r\n");
// 	} else {
// 		char buffer [33];
// 		sprintf(buffer, "%d", m_contentServed);
// 		request.append("Range: bytes=");
// 		request.append(buffer);
// 		request.append("-\r\n");
// 
// 	}

	eHttpConnection* fd = new eHttpConnection();
	m_rbuffer.reset();

	for (int i = 0; i < 3; i++)
	{
		int rc = fd->openUrl(currenturl);
		if ( rc < 0 ) {
			eDebug("%s: connection failed (%m)", __FUNCTION__);
			return rc;
		}
		
		if (fd->statuscode() == 302) {
			currenturl = fd->findHeader("Location: ");
			fd->close();
			if (currenturl.empty()) {
				eDebug("%s: connection failed (302 but no new url)", __FUNCTION__);
				return -1;
			} else {
				eDebug("%s: redirecting to: %s", __FUNCTION__, currenturl.c_str());
				continue;
			}
		}
		
		m_chunkedTransfer = (fd->findHeader("Transfer-Encoding: ") == "chunked");
		
		std::string value = fd->findHeader("Content-Length: ");
		if (!value.empty()) {
			m_contentLength = strtol(value.c_str(), NULL, 16);
		}
		
		std::string contentStr = fd->findHeader("Content-Type: ");
		std::transform(contentStr.begin(), contentStr.end(),contentStr.begin(), tolower );

		/* assume we'll get a playlist, some text file containing a stream url */
		const bool playlist = (contentStr.find("application/text") != std::string::npos
				|| contentStr.find("audio/x-mpegurl")  != std::string::npos
				|| contentStr.find("audio/mpegurl")    != std::string::npos
				|| contentStr.find("application/m3u")  != std::string::npos);
		if (playlist) {
			// TODO
// 			m_header = m_header.substr(pos+1);
// 			pos = m_header.find("http://");
// 			if (pos != std::string::npos) {
// 				newurl = m_header.substr(pos);
// 				pos = m_header.find("\n");
// 				if (pos != std::string::npos) newurl = newurl.substr(0, pos);
// 				eDebug("%s: playlist entry: %s", __FUNCTION__, newurl.c_str());
// 				return 0;
// 			}
		}
		
		m_ishls = (contentStr.find("application/vnd.apple.mpegurl") != std::string::npos
		        || contentStr.find("audio/x-mpegurl") != std::string::npos
		        || contentStr.find("application/x-mpegurl") != std::string::npos);
		if (m_ishls) {
			eDebug("HLS stream detected");
			readPlaylist(fd->fd());
			fd->close();
			return 0;
		}
	

		if (m_chunkedTransfer) {
			eDebug("%s: chunked transfer enabled", __FUNCTION__);
			if (m_scratch == NULL) { m_scratchSize=64; m_scratch=(char*)malloc(m_scratchSize);}
	// 		int c = eSocketBase::readLine(m_streamSocket, &m_scratch, &m_scratchSize);
	// 		if (c <=0) return -1;
	// 		m_chunkSize = strtol(m_scratch, NULL, 16);
	// 		eDebug("%s: chunked transfer enabled, fisrt chunk size %i", __FUNCTION__, m_chunkSize);
		}

		
// 	ssize_t toWrite = m_rbuffer.availableToWritePtr();
// 	if (toWrite > 0) {
// 		if (m_chunkedTransfer) toWrite = MIN(toWrite, m_chunkSize);
// 		toWrite = eSocketBase::timedRead(m_streamSocket, m_rbuffer.ptr(), toWrite, 2000, 50);
// 		if (toWrite > 0) {
// 			eDebug("%s: writting %i bytes to the ring buffer", __FUNCTION__, toWrite);
//                         ssize_t skipBytes = 0;
//                         char* ptr = m_rbuffer.ptr();
//                         while (skipBytes <= toWrite && ptr[skipBytes] != 0x47) ++skipBytes;
// 			m_rbuffer.ptrWriteCommit(toWrite);
// 			m_rbuffer.skip(skipBytes);
// 
// 			if (m_chunkedTransfer) {
// 				m_chunkSize -= toWrite;
//                                 if (m_chunkSize==0){
//                                         int rc = eSocketBase::readLine(m_streamSocket, &m_scratch, &m_scratchSize);
//                                         eDebug("%s: reading the end of the chunk rc(%i)(%s)", __FUNCTION__, rc, m_scratch);
//                                 }
// 			}
// 		}
// 	}
	
// 			if (m_ishls) {
// 			std::string data;
// 			char buf[1024];
// 			int c;
// 			while ((c = eSocketBase::timedRead(m_streamSocket, buf, 1024, 1000)) > 0) {
// 				eDebug("m3u8 len += %d", c);
// 				buf[c] = '\0';
// 				data.append(buf);
// 			}
// 			eDebug("m3u8 %d\n%s", c, data.c_str());
// 			parsePlaylist(data);
// 		}
	
		eDebug("%s: started streaming...", __FUNCTION__);
		m_streamSocket = fd->fd();
		return 0;
	}
	
	eDebug("%s: too many redirects...", __FUNCTION__);
	/* too many redirect / playlist levels (we accept one redirect + one playlist) */
	close();
	return -1;
}

int eHttpConnection::close()
{
	eDebug("eHttpConnection:%s", __FUNCTION__);
	if (m_fd > 0) {
		::close(m_fd);
	}
	m_statuscode = -1;
	m_header = "";
}


int eHttpConnection::openUrl(const std::string& url)
{
	eDebug("%s: %s", __FUNCTION__, url.c_str());
	close();
	
	int port;
	std::string hostname;
	std::string authorizationData;
	std::string uri = url;
	std::string request;
	int result;
	char proto[100];
	char statusmsg[100];

	int pathindex = uri.find("/", 7);
	if (pathindex > 0)
	{
		hostname = uri.substr(7, pathindex - 7);
		uri = uri.substr(pathindex, uri.length() - pathindex);
	}
	else
	{
		hostname = uri.substr(7, uri.length() - 7);
		uri = "";
	}
	int authenticationindex = hostname.find("@");
	if (authenticationindex > 0)
	{
		BIO *mbio, *b64bio, *bio;
		char *p = (char*)NULL;
		authorizationData = hostname.substr(0, authenticationindex);
		hostname = hostname.substr(authenticationindex + 1);
		mbio = BIO_new(BIO_s_mem());
		b64bio = BIO_new(BIO_f_base64());
		bio = BIO_push(b64bio, mbio);
		BIO_write(bio, authorizationData.data(), authorizationData.length());
		BIO_flush(bio);
		int length = BIO_ctrl(mbio, BIO_CTRL_INFO, 0, (char*)&p);
		authorizationData = "";
		if (p && length > 0)
		{
			/* base64 output contains a linefeed, which we ignore */
			authorizationData.append(p, length - 1);
		}
		BIO_free_all(bio);
	}
	int customportindex = hostname.find(":");
	if (customportindex > 0)
	{
		port = atoi(hostname.substr(customportindex + 1, hostname.length() - customportindex - 1).c_str());
		hostname = hostname.substr(0, customportindex);
	}
	else if (customportindex == 0)
	{
		port = atoi(hostname.substr(1, hostname.length() - 1).c_str());
		hostname = "localhost";
	}
	else
	{
		port = 80;
	}

	m_fd = eSocketBase::connect(hostname.c_str(), port, 100);
	if (m_fd < 0) return -1;

	request = "GET ";
	request.append(uri).append(" HTTP/1.1\r\n");
	request.append("Host: ").append(hostname).append("\r\n");
	if (authorizationData.length())
	{
		request.append("Authorization: Basic ").append(authorizationData).append("\r\n");
	}
	request += m_request;
	request.append("Accept: */*\r\n");
	
	request.append("Connection: close\r\n");
	request.append("\r\n");

	
	result = eSocketBase::openHTTPConnection(m_fd, request, m_header);
	if (result < 0) return -1;
	
	eDebug("== HEADER ==\n%s", m_header.c_str());

	size_t pos = m_header.find("\n");
	const std::string& httpStatus = (pos == std::string::npos)? m_header: m_header.substr(0, pos);

	result = sscanf(httpStatus.c_str(), "%99s %3d %99s", proto, &m_statuscode, statusmsg);
	if (result != 3 || (m_statuscode != 200 && m_statuscode != 206 && m_statuscode != 302))
	{
		eDebug("%s: wrong http response code: %d", __FUNCTION__, m_statuscode);
		return -1;
	}

	m_header = m_header.substr(pos+1);
	
	return 0;
}


bool strstart(const std::string &str, const std::string &prefix, std::string *left)
{
	if (str.compare(0, prefix.size(), prefix) == 0) {
		*left = str.substr(prefix.size());
		return true;
	} else {
		return false;
	}
}

int eHttpStream::readPlaylist(int fd)
{
	eDebug("%s", __FUNCTION__);
	std::string data;
	char buf[1024];
	int c;
	while ((c = eSocketBase::timedRead(fd, buf, sizeof(buf)-1, 1000)) > 0) {
// 		eDebug("m3u8 len += %d", c);
		buf[c] = '\0';
		data.append(buf);
	}
	eDebug("M3U8 %d\n%s", c, data.c_str());
	parsePlaylist(data);
}


int eHttpStream::parsePlaylist(std::string s)
{
	std::string line;
	std::string value;
	std::istringstream stream(s);
	int target_duration = 0;
	int duration = 0;
	bool skip = true;
	int seq_no = 0;
	
	std::getline(stream, line);
	if (!strstart(line, "#EXTM3U", &value)) { return -1; }

	while (std::getline(stream, line)) {
		eDebug("process_line %s", line.c_str());
		std::string prefix = "#EXT-X-TARGETDURATION:";
		if (line.compare(0, prefix.size(), prefix) == 0) {
			eDebug("%s", line.substr(prefix.size()).c_str() );
			target_duration = atoi( line.substr(prefix.size()).c_str() );
		}
		prefix = "#EXT-X-MEDIA-SEQUENCE:";
		if (line.compare(0, prefix.size(), prefix) == 0) {
			seq_no = atoi( line.substr(prefix.size()).c_str() );
		}
		if (strstart(line, "#EXTINF:", &value)) {
			duration = atoi(value.c_str());
			eDebug("duration %d", duration);
// 			total_duration += duration;
		}
		if (strstart(line, "#", &value)) {
			continue;
		} else {
			if (!skip || m_lastSegmentUrl.empty()) {
				eDebug("push segment url %s", line.c_str());
				struct hls_segment segment;
				segment.url = line;
				segment.duration = duration;
				m_segments.push(segment);
			}
			if (skip && (m_lastSegmentUrl.empty() || m_lastSegmentUrl == line) ) {
				eDebug("concat at %s", line.c_str());
				skip = false;
			}

		}
	}
	
	m_lastSegmentUrl = m_segments.back().url;
	
	if (seq_no == m_lastMediaSequence) {
		eDebug("same MEDIA-SEQUENCE %d", seq_no);
		m_nextLoadTime += m_segments.back().duration;
	}
	else if (seq_no != m_lastMediaSequence + 1) {
		eDebug("jump to MEDIA-SEQUENCE %d", seq_no);
		m_nextLoadTime += MIN(m_segments.back().duration, target_duration);
		if (target_duration == 0) { m_nextLoadTime += m_segments.back().duration; }
	}
	else {
		eDebug("MEDIA-SEQUENCE %d", seq_no );
		m_nextLoadTime += MIN(m_segments.back().duration, target_duration);
		if (target_duration == 0) { m_nextLoadTime += m_segments.back().duration; }
	}
	
	m_lastMediaSequence = seq_no;
	return 0;
	
}

std::string eHttpStream::prefixSegment(const std::string &segment)
{
	if (segment.compare(0, 7, "http://") == 0) {
		return segment;
	}
	size_t pos = m_url.rfind("/");
	return m_url.substr(0, pos+1) + segment;
}

int eHttpStream::close()
{
	eDebug(__FUNCTION__);
	int retval = -1;
	if (m_streamSocket >= 0)
	{
		retval = ::close(m_streamSocket);
		m_streamSocket = -1;
	}
	// no reconnect attempts after close
	m_tryToReconnect = false;
	return retval;
}

ssize_t eHttpStream::read(off_t offset, void *buf, size_t count)
{
	if (m_streamSocket < 0) {
		eDebug("%s: not valid fd", __FUNCTION__);
		if (openSock(m_url) < 0) return -1;
	}

// 	bool outBufferHasData = false;
// 	if (m_rbuffer.availableToRead() >= count) {
//         	m_contentServed += m_rbuffer.read((char*)buf, (count - (count%188)));
// 		outBufferHasData = true;
// 	}
// 	int read2Chunks=2;
// READAGAIN:
// 	ssize_t toWrite = m_rbuffer.availableToWritePtr();
// 	eDebug("%s: Ring buffer available to write %i", __FUNCTION__, toWrite);
// 	if (toWrite > 0) {
// 
// 		if (m_chunkedTransfer){
// 			if (m_chunkSize==0) {
// 				int c = eSocketBase::readLine(m_streamSocket, &m_scratch, &m_scratchSize);
// 				if (c <= 0) return -1;
// 				m_chunkSize = strtol(m_scratch, NULL, 16);
// 				if (m_chunkSize == 0) return -1;
// 			}
// 			toWrite = MIN(toWrite, m_chunkSize);
// 		}
// 		if (outBufferHasData || m_rbuffer.availableToRead() >= (1024*6) || m_rbuffer.availableToRead() >= count) {
// 			//do not starve the reader if we have enough data to read and there is nothing on the socket
// 			toWrite = eSocketBase::timedRead(m_streamSocket, m_rbuffer.ptr(), toWrite, 0, 0);
// 		} else {
// 			toWrite = eSocketBase::timedRead(m_streamSocket, m_rbuffer.ptr(), toWrite, 3000, 500);
// 		}
// 		if (toWrite > 0) {
// 			m_rbuffer.ptrWriteCommit(toWrite);
// 			if (m_chunkedTransfer) {
//                                 --read2Chunks;
// 				m_chunkSize -= toWrite;
// 				if (m_chunkSize==0){
// 					eSocketBase::readLine(m_streamSocket, &m_scratch, &m_scratchSize);
// 				} else if (read2Chunks > 0) {
// 					goto READAGAIN;
// 				}
// 			}
// 			//try to reconnect on next failure
// 			m_tryToReconnect = true;
// 		} else if (!outBufferHasData && m_rbuffer.availableToRead() < 188) {
// 			eDebug("%s: failed to read from the socket errno(%i) timedRead(%i)...", __FUNCTION__, errno, toWrite);
// 			// we failed to read and there is nothing to play, try to reconnect?
// 			// for some reason select() returns EAGAIN sometimes
// 			// do not reconnect if sream has a content length and all lentgh is served
// 			if ((toWrite <= 0 || errno == EAGAIN) && m_tryToReconnect && (!m_contentLength || m_contentLength > m_contentServed)) {
// 				m_tryToReconnect = false;
// 				if (_open(m_url) == 0) {
// 					goto READAGAIN;
// 				}
// 			} else if (toWrite == 0 && (!m_contentLength || m_contentLength > m_contentServed)) {
// 				errno = EAGAIN; //timeout
// 			} else close();
// 
// 			eDebug("%s: timed out", __FUNCTION__);
// 			return -1 ;
// 		}
// 	}
// 	
	int timems;
	if (m_rbuffer.availableToRead() >= count || m_rbuffer.availableToRead() >= (1024*500) ) {
		//do not starve the reader if we have enough data to read and there is nothing on the socket
		timems = 1;
	} else {
		timems = 3000;
	}
	int maxWrite = MIN(m_rbuffer.availableToWritePtr(), RBUFFER_SIZE/3);
	
	eDebug("fillbuff");
	int toWrite;
	while (m_rbuffer.availableToWritePtr() > 0 && maxWrite > 0 && timems > 0) {
		toWrite = MIN(m_rbuffer.availableToWritePtr(), maxWrite);
		
		if (m_chunkedTransfer) {
			if (m_chunkSize == 0) {
				int c = eSocketBase::readLine(m_streamSocket, &m_scratch, &m_scratchSize, &timems);
				if (c > 0) {
					m_chunkSize = strtol(m_scratch, NULL, 16);
					if (m_chunkSize == 0) break;
				} else continue;
			}
			toWrite = MIN(toWrite, m_chunkSize);
		}
		
		if (m_ishls) {
			eDebug("m_chunkSize %d", m_chunkSize);
			//TODO correct check if need update
			if (m_segments.empty() || m_nextLoadTime > time(NULL)) {
				eDebug("loading m3u8");
				eHttpConnection conn;
				int rc = conn.openUrl(m_url);
				if (rc < 0) {
					break;
				}
				readPlaylist(conn.fd());
				conn.close();
				continue;
			}
			
			if (m_chunkSize == 0) {
				eDebug("pop segment %s", m_segments.front().url.c_str());
				eHttpConnection conn;
				int rc = conn.openUrl(prefixSegment(m_segments.front().url));
				if (rc < 0) { break; }
				m_streamSocket = conn.fd();
				m_segments.pop();
				std::string value = conn.findHeader("Content-Length: ");
				m_chunkSize = 0;
				if (!value.empty()) {
					m_chunkSize = strtol(value.c_str(), NULL, 10);
				}
			}
			
			toWrite = MIN(toWrite, m_chunkSize);
		}
		
		toWrite = eSocketBase::timedRead(m_streamSocket, m_rbuffer.ptr(), toWrite, &timems);
		if (toWrite > 0) {
			eDebug("%s: writting %i bytes to the ring buffer", __FUNCTION__, toWrite);
			ssize_t skipBytes = 0;
// 			if (m_firstOffset) {
				char* ptr = m_rbuffer.ptr();
				while (skipBytes <= toWrite) {
					if (ptr[skipBytes] == 0x47) {
						m_firstOffset = false;
						break;
					}
					++skipBytes;
				}
				eDebug("%s: skipping %i bytes", __FUNCTION__, skipBytes);
// 			}
			
			m_rbuffer.ptrWriteCommit(toWrite);
			maxWrite -= toWrite;
			
			if (m_firstOffset) {
				m_rbuffer.skip(skipBytes);
			}
			
			if (m_chunkedTransfer || m_ishls) {
				m_chunkSize -= toWrite;
			}
		} else break;
//		eDebug("time %d", timems);
	}


	ssize_t toRead = MIN(count, m_rbuffer.availableToRead());
	toRead = m_rbuffer.read((char*)buf, (toRead - (toRead%188)));
	m_contentServed += toRead;
	return toRead;
}

int eHttpStream::valid()
{
//	useless for non-blocking
	return 1;
//	return (m_streamSocket != -1);
}

off_t eHttpStream::length()
{
	if (m_contentLength > 0) {
		return (off_t)m_contentLength;
	} else {
		return (off_t)-1;
	}
}

off_t eHttpStream::offset()
{
	return 0;
}


