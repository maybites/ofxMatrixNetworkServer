#include "ofxKinectNetworkServer.h"


//------------------------------------------------------------------------------
ofxKinectNetworkServer::ofxKinectNetworkServer()
{
}

//------------------------------------------------------------------------------
ofxKinectNetworkServer::~ofxKinectNetworkServer()
{
}

//--------------------------------------------------------------
void ofxKinectNetworkServer::update() {
    //for each client lets send them a message letting them know what port they are connected on
	for(int i = 0; i < getLastID(); i++){
		if(isClientConnected(i)){
            //if we don't have a string allocated yet: lets create one
            //if(i >= storeText.size() ){
            //    storeText.push_back( string() );
            //}
            
            //if we don't have a string allocated yet: lets create one
            if(i >= tx_valid.size() ){
                tx_valid.push_back(0);
                tx_rawframe.push_back(0);
                tx_rgbframe.push_back(0);
                tx_depthframe.push_back(0);
            }
            
            //we only want to update the text we have recieved there is data
            string str = receive(i);
            
            if(str == "handshake"){
                ofLog(OF_LOG_NOTICE, "handshake with client " + ofToString(i));
                sendHandshake(i);
                tx_valid[i] = 1;
            }else if(str == "rawframe"){
                ofLog(OF_LOG_NOTICE, "rawframe for client " + ofToString(i) + " - " + str);
                tx_rawframe[i] = 1;
            }else if(str == "rgbframe"){
                ofLog(OF_LOG_NOTICE, "rgbframe for client " + ofToString(i) + " - " + str);
                tx_rgbframe[i] = 1;
            }else if(str == "depthframe"){
                ofLog(OF_LOG_NOTICE, "depthframe for client " + ofToString(i) + " - " + str);
                tx_depthframe[i] = 1;
            }else if(str == "disconnect"){
                ofLog(OF_LOG_NOTICE, "disconnect client " + ofToString(i));
                sendDisconnect(i);
                disconnectClient(i);
                tx_valid[i] = 0;
            }
            
            //if(str.length() > 0){
            //    storeText[i] = str;
            //}
        }else{
            tx_valid[i] = 0;
        }
	}
}

//--------------------------------------------------------------
void ofxKinectNetworkServer::draw(int xRefPos, int yRefPos){
    
	ofSetHexColor(0xDDDDDD);
	ofDrawBitmapString("Matrix SERVER connect on port: "+ofToString(getPort()), xRefPos, yRefPos);
    
	//ofSetHexColor(0x000000);
	//ofRect(10, 60, ofGetWidth()-24, ofGetHeight() - 65 - 15);
    
	ofSetHexColor(0xDDDDDD);
    
	//for each connected client lets get the data being sent and lets print it to the screen
	for(unsigned int i = 0; i < (unsigned int)getLastID(); i++){
        
		if( !isClientConnected(i) )continue;
        
		//give each client its own color
		ofSetColor(255 - i*30, 255 - i * 20, 100 + i*40);
        
		//calculate where to draw the text
		int xPos = xRefPos + 5;
		int yPos = yRefPos + 15 + (12 * i * 4);
        
		//get the ip and port of the client
		string port = ofToString( getClientPort(i) );
		string ip   = getClientIP(i);
		string info = "client "+ofToString(i)+" -connected from "+ip+" on port: "+port;
        
		//draw the info text and the received text bellow it
		ofDrawBitmapString(info, xPos, yPos);
		//ofDrawBitmapString(storeText[i], 25, yPos + 10);
        
	}
    
}

//--------------------------------------------------------------
void ofxKinectNetworkServer::exit() {
    //for each client lets send them a message letting them know what port they are connected on
	for(int i = 0; i < getLastID(); i++){
		if(isClientConnected(i)){
            sendExit(i);
            disconnectClient(i);
        }
	}
    
    close();
}

//------------------------------------------------------------------------------
void ofxKinectNetworkServer::sendDepthFrame(const ofPixelsRef pixels){
    int kintype = KIN_MATRIX_TYPE_DEPTH;
	//for each connected client lets get the data being sent and lets print it to the screen
	for(unsigned int i = 0; i < (unsigned int)getLastID(); i++){
		if(isClientConnected(i) && tx_depthframe[i] == 1){
            tx_depthframe[i] == 0;
            sendCharFrame(pixels, kintype, i);
        }
    }
}

//------------------------------------------------------------------------------
void ofxKinectNetworkServer::sendRGBFrame(const ofPixelsRef pixels){
    int kintype = KIN_MATRIX_TYPE_RGB;
 	//for each connected client lets get the data being sent and lets print it to the screen
	for(unsigned int i = 0; i < (unsigned int)getLastID(); i++){
		if(isClientConnected(i) && tx_rgbframe[i] == 1){
            tx_rgbframe[i] == 0;
            sendCharFrame(pixels, kintype, i);
        }
    }
}

//------------------------------------------------------------------------------
void ofxKinectNetworkServer::sendRAWFrame(const ofShortPixelsRef pixels){
    int kintype = KIN_MATRIX_TYPE_RAW;
    //for each connected client lets get the data being sent and lets print it to the screen
    for(unsigned int i = 0; i < (unsigned int)getLastID(); i++){
        if(isClientConnected(i) && tx_rawframe[i] == 1){
            tx_rawframe[i] == 0;
            sendShortFrame(pixels, kintype, i);
        }
    }
}

//------------------------------------------------------------------------------
void ofxKinectNetworkServer::sendCharFrame(const ofPixelsRef pixels, int kinecttype, int clientid)
{
    int planecount = pixels.getNumChannels();
    int dimcount = 2; // only sending 2d matrices from of
    int dim[dimcount];
    dim[0]       = pixels.getWidth();
    dim[1]       = pixels.getHeight();
    int typeSize = pixels.getBytesPerChannel();
    int type     = JIT_MATRIX_TYPE_CHAR;
    
    makeMatrixHeader(planecount, typeSize, kinecttype, type, dim, dimcount);
    
    char *matrix = (char*)pixels.getPixels();
    
    
    //////SEND ONE MATRIX
    sendRawBytes(clientid, (char *)(&m_chunkHeader), sizeof(t_jit_net_packet_header));
    sendRawBytes(clientid, (char *)(&m_matrixHeader), sizeof(t_jit_net_packet_matrix));
    
    //DELETE THIS LINE
    //int packSize = SWAP32(m_matrixHeader.dimstride[dimcount-1])*SWAP32(m_matrixHeader.dim[dimcount-1]);
    
    //ofLog(OF_LOG_NOTICE, "send frame to client: " + ofToString(i));
    int vector = dim[0] * typeSize * planecount;
    for(int j = 0; j < dim[1]; j++)
        sendRawBytes(clientid, matrix + j * vector, vector);
}


//------------------------------------------------------------------------------
void ofxKinectNetworkServer::sendShortFrame(const ofShortPixelsRef pixels, int kinecttype, int clientid)
{
    int planecount = pixels.getNumChannels();
    int dimcount = 2; // only sending 2d matrices from of
    int dim[dimcount];
    dim[0]       = pixels.getWidth();
    dim[1]       = pixels.getHeight();
    int typeSize = pixels.getBytesPerChannel();
    int type     = JIT_MATRIX_TYPE_SHORT;
    
    makeMatrixHeader(planecount, typeSize, kinecttype, type, dim, dimcount);
    
    char *matrix = (char*)pixels.getPixels();
    
    
    //////SEND ONE MATRIX
    sendRawBytes(clientid, (char *)(&m_chunkHeader), sizeof(t_jit_net_packet_header));
    sendRawBytes(clientid, (char *)(&m_matrixHeader), sizeof(t_jit_net_packet_matrix));
    
    //DELETE THIS LINE
    //int packSize = SWAP32(m_matrixHeader.dimstride[dimcount-1])*SWAP32(m_matrixHeader.dim[dimcount-1]);
    
    //ofLog(OF_LOG_NOTICE, "send frame to client: " + ofToString(i));
    int vector = dim[0] * typeSize * planecount;
    for(int j = 0; j < dim[1]; j++){
        sendRawBytes(clientid, matrix + j * vector, vector);
    }
}


//------------------------------------------------------------------------------
void ofxKinectNetworkServer::sendHandshake(int i)
{
    //////SEND HANDSHAKE
    
    m_chunkHeader.id = SWAP32(JIT_MESSAGE_HANDSHAKE_ID);
    m_chunkHeader.size = 0;
    
    sendRawBytes(i, (char *)(&m_chunkHeader), sizeof(t_jit_net_packet_header));
}

//------------------------------------------------------------------------------
void ofxKinectNetworkServer::sendExit(int i)
{
    //////SEND HANDSHAKE
    
    m_chunkHeader.id = SWAP32(JIT_MESSAGE_EXIT_ID);
    m_chunkHeader.size = 0;
    
    sendRawBytes(i, (char *)(&m_chunkHeader), sizeof(t_jit_net_packet_header));
}

//------------------------------------------------------------------------------
void ofxKinectNetworkServer::sendDisconnect(int i)
{
    //////SEND HANDSHAKE
    
    m_chunkHeader.id = SWAP32(JIT_MESSAGE_DISCONNECT_ID);
    m_chunkHeader.size = 0;
    
    sendRawBytes(i, (char *)(&m_chunkHeader), sizeof(t_jit_net_packet_header));
}

//------------------------------------------------------------------------------
void ofxKinectNetworkServer::sendText(const string& txt) {
	//for each connected client lets get the data being sent and lets print it to the screen
	for(unsigned int i = 0; i < (unsigned int)getLastID(); i++){
        
		if( !isClientConnected(i) )continue;
        m_messageHeader.id = SWAP32(JIT_MESSAGE_PACKET_ID);
        m_messageHeader.size = SWAP32(sizeof(long) + // size
                                      sizeof(long) + // ac
                                      sizeof(char) + // type
                                      sizeof(char)*txt.length() + // number
                                      sizeof(char)); // null terminator
        
        sendRawBytes(i, (char *)&m_messageHeader.id, sizeof(long));
        sendRawBytes(i, (char *)&m_messageHeader.size, sizeof(long));
        
        // the packet
        long messageSizeBytes = m_messageHeader.size; //	32-bit integer that contains the size of the serialized message in bytes. 
        long ac = SWAP32(0);      //    Following that another 32-bit integer gives the argument count for the atoms. 
        /// Following that comes the message atoms themselves, starting with the leading symbol if it exists. 
        //  Each atom is represented in memory first with a char that indicates what type of atom it is:
        //		's' for symbol, 'l' for long, and 'f' for float. 
        //		For long and float atoms, the next 4 bytes contain the value of the atom; 
        //		for symbol atoms a null terminated character string follows. 
        
        
        char atomType = 's'; //'s' for symbol, 'l' for long, and 'f' for float. 
        const char *cp = txt.c_str(); // seriously
        char nullTerm = '\0';
        sendRawBytes(i, (char *)&messageSizeBytes, sizeof(long));
        sendRawBytes(i, (char *)&ac, sizeof(long));
        sendRawBytes(i, (char *)&atomType, sizeof(char));
        sendRawBytes(i, (char *)cp, txt.length()*sizeof(char));
        sendRawBytes(i, (char *)&nullTerm, sizeof(char));
        //readResponse();
    }
}

//------------------------------------------------------------------------------
double ofxKinectNetworkServer::getLastSent() const
{
    return lastSent;
}

//------------------------------------------------------------------------------
void ofxKinectNetworkServer::readResponse()
{
 	//for each connected client lets get the data being sent and lets print it to the screen
	for(unsigned int i = 0; i < (unsigned int)getLastID(); i++){
        
		if( !isClientConnected(i) )continue;

        // TODO read latency data here.
        char buf[MAXDATASIZE]; 
        int numBytes = receiveRawBytes(i, buf, MAXDATASIZE-1);
        if (numBytes == -1)
        {
            // printf("recv error\n");
            // skip it, there's nothing there
        }
        else
        {
            buf[numBytes] = '\0'; // end it
            
            m_latencyPacket.id                      = ((t_jit_net_packet_latency *)buf)->id; // cast it to get the id
            m_latencyPacket.client_time_original    = ((t_jit_net_packet_latency *)buf)->client_time_original;
            m_latencyPacket.server_time_before_data = ((t_jit_net_packet_latency *)buf)->server_time_before_data;
            m_latencyPacket.server_time_after_data  = ((t_jit_net_packet_latency *)buf)->server_time_after_data;
            
    //printf("id: %d\n", (m_latencyPacket.id));
    //printf("client time original %f\n",m_latencyPacket.client_time_original);
    //printf("before Data %fl\n",m_latencyPacket.server_time_before_data);
    //printf("after Data %f\n",m_latencyPacket.server_time_after_data);
    //printf("diff=%f\n\n",m_latencyPacket.server_time_after_data - m_latencyPacket.server_time_before_data);
            
            // cout << buf << endl;
            
            // if(lastSent >= m_latencyPacket.client_time_original) {
            //  printf("GTOE => last sent=%f and client_time_original=%f\n",lastSent,m_latencyPacket.client_time_original);	
            // } else {
            //  printf("NNNWWW => last sent=%f and client_time_original=%f\n",lastSent,m_latencyPacket.client_time_original);	
            // }
        }
    }
}

//------------------------------------------------------------------------------
void ofxKinectNetworkServer::makeMatrixHeader(int planecount,
                                              int typeSize,
                                              int kinecttype,
                                              int type,
                                              int *dim,
                                              int dimcount)
{
    long i, j, k;
    
    m_chunkHeader.id = SWAP32(JIT_MESSAGE_PACKET_ID);
    m_chunkHeader.size = SWAP32(sizeof(t_jit_net_packet_matrix));
    
    m_matrixHeader.id = SWAP32(JIT_MATRIX_PACKET_ID);
    m_matrixHeader.size = SWAP32(sizeof(t_jit_net_packet_matrix));
    m_matrixHeader.planecount = SWAP32(planecount);
    m_matrixHeader.kinecttype = SWAP32(kinecttype);
    m_matrixHeader.type = SWAP32(type);
    m_matrixHeader.dimcount = SWAP32(dimcount);
    
    for(i=0; i < dimcount; i++)
    {
        m_matrixHeader.dim[i] = SWAP32(dim[i]);
    }
    
    while(i < JIT_MATRIX_MAX_DIMCOUNT)
    {
        m_matrixHeader.dim[i] = SWAP32(0); // <-- in the jitter one, they seem to just copy the dim through ...
        i++;
    }
    
    //special case for first value
    m_matrixHeader.dimstride[0] = SWAP32(typeSize * planecount);
    
    for(i=1; i <= dimcount; i++)
    {
        m_matrixHeader.dimstride[i] = SWAP32(dim[i-1]*SWAP32(m_matrixHeader.dimstride[i-1])); // watch out for these .. they need to come back ...
    }
    
    while(i < JIT_MATRIX_MAX_DIMCOUNT)
    {
        m_matrixHeader.dimstride[i] = SWAP32(0);
        i++;
    }

    m_matrixHeader.datasize = SWAP32(SWAP32(m_matrixHeader.dimstride[dimcount-1])*SWAP32(m_matrixHeader.dim[dimcount-1]));
    m_matrixHeader.time = ofGetElapsedTimef();

    // just to keep track
    lastSent = m_matrixHeader.time;

}


