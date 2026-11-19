// Suporte microSD detectado no arranque, SPI CS=D10 (configuravel).
// Sem modulo/cartao, o firmware continua em RAM e no navegador.
// Nunca escrever no cartao dentro das ISR de aquisicao/transicao.
#ifndef ODL_ENABLE_SD
#define ODL_ENABLE_SD 1
#endif
#ifndef ODL_SD_CS_PIN
#define ODL_SD_CS_PIN 10
#endif
#if ODL_ENABLE_SD
#include <SPI.h>
#include <SD.h>
#endif
bool sdReady=false;
bool sdLogging=false;
char sdFilename[13]="";
uint32_t sdBytesWritten=0;
uint32_t sdWriteErrors=0;
uint32_t sdLastEdgeSeq=0;
uint32_t sdMissedEdges=0;
char sdWriteBuffer[512];
uint16_t sdWriteUsed=0;
uint32_t sdLastFlushMs=0;
#if ODL_ENABLE_SD
File sdFile;
#endif
void sdFlush(){
#if ODL_ENABLE_SD
  if(!sdLogging||!sdFile||!sdWriteUsed)return;
  const size_t n=sdFile.write((const uint8_t*)sdWriteBuffer,sdWriteUsed);
  if(n!=sdWriteUsed)++sdWriteErrors;
  sdBytesWritten+=n;sdWriteUsed=0;sdLastFlushMs=millis();
#endif
}
void sdAppend(const char *text){
#if ODL_ENABLE_SD
  if(!sdLogging)return;
  const size_t length=strlen(text);
  if(length>sizeof(sdWriteBuffer)){++sdWriteErrors;return;}
  if(sdWriteUsed+length>sizeof(sdWriteBuffer))sdFlush();
  memcpy(sdWriteBuffer+sdWriteUsed,text,length);
  sdWriteUsed+=length;
#else
  (void)text;
#endif
}
void sdLogFrame(const CaptureFrame &frame){
#if ODL_ENABLE_SD
  if(!sdLogging)return;
  char line[180];
  size_t offset=(size_t)snprintf(line,sizeof(line),"S;%lu;",(unsigned long)frame.timeMs);
  const uint32_t sec=epochAnchor ? epochAnchor+(uint32_t)((int32_t)(frame.timeMs-millisAnchor)/1000L):0;
  offset+=(size_t)snprintf(line+offset,sizeof(line)-offset,"%lu;",(unsigned long)sec);
  for(uint8_t i=0;i<activeCount && offset+12<sizeof(line);++i){
    offset+=(size_t)snprintf(line+offset,sizeof(line)-offset,"%u;",(unsigned)frame.values[i]);
  }
  if(offset+2<sizeof(line)){line[offset++]='\n';line[offset]=0;sdAppend(line);}
#else
  (void)frame;
#endif
}
void sdService(){
#if ODL_ENABLE_SD
  if(!sdLogging)return;
  // Retirar UM evento por secao critica, em O(1), sem procurar 128 entradas
  // com as interrupcoes desactivadas: a aquisicao mantem a prioridade.
  for(uint8_t n=0;n<16;++n){
    DigitalEdge found={};bool available=false;
    noInterrupts();
    const uint8_t count=digitalEdgeUsed;
    if(count){
      const uint8_t first=(uint8_t)((digitalEdgeHead+DIGITAL_EVENT_CAPACITY-count)%DIGITAL_EVENT_CAPACITY);
      const uint32_t oldest=digitalEdges[first].sequence;
      uint32_t wanted=sdLastEdgeSeq+1;
      if(wanted<oldest){sdMissedEdges+=(oldest-wanted);wanted=oldest;}
      const uint32_t index=wanted-oldest;
      if(index<count){found=digitalEdges[(first+index)%DIGITAL_EVENT_CAPACITY];available=true;}
    }
    interrupts();
    if(!available)break;
    char line[96];
    snprintf(line,sizeof(line),"E;%lu;%lu;%u;%u;%u\n",
      (unsigned long)found.timeMs,(unsigned long)found.sequence,
      (unsigned)found.microPart,(unsigned)found.channel,(unsigned)found.level);
    sdAppend(line);sdLastEdgeSeq=found.sequence;
  }
  if(sdWriteUsed && millis()-sdLastFlushMs>=2000UL){sdFlush();sdFile.flush();}
#endif
}
void sdSetup(){
#if ODL_ENABLE_SD
  sdReady=SD.begin(ODL_SD_CS_PIN);
  Serial.println(sdReady?"SD detectado no arranque":"SD nao detectado; modo RAM/Safari");
#else
  sdReady=false;
#endif
}
void sdStop(){
#if ODL_ENABLE_SD
  if(sdLogging){sdService();sdFlush();sdFile.flush();sdFile.close();}
#endif
  sdLogging=false;
}
bool sdStart(){
#if ODL_ENABLE_SD
  if(!sdReady||sdLogging)return false;
  for(unsigned i=0;i<10000;++i){
    snprintf(sdFilename,sizeof(sdFilename),"ODL%04u.CSV",i);
    if(!SD.exists(sdFilename))break;
    if(i==9999)return false;
  }
  sdFile=SD.open(sdFilename,FILE_WRITE);
  if(!sdFile){++sdWriteErrors;return false;}
  sdLogging=true;sdWriteUsed=0;sdBytesWritten=0;sdMissedEdges=0;
  noInterrupts();sdLastEdgeSeq=digitalEdgeSerial;interrupts();
  sdAppend("# OpenDataLogger 1.0.1;S=amostra;E=flanco;formato=raw\n");
  char meta[128];
  snprintf(meta,sizeof(meta),"# sample_ms=%lu;epoch_anchor=%lu;clock_ms=%lu;channels=",
    (unsigned long)sampleMs,(unsigned long)epochAnchor,(unsigned long)millisAnchor);
  sdAppend(meta);
  for(uint8_t i=0;i<activeCount;++i){
    snprintf(meta,sizeof(meta),"%s%u",i?",":"",(unsigned)activeIds[i]);sdAppend(meta);
  }
  sdAppend("\n# S;uptime_ms;unix_s;raw_0;raw_1;...\n# E;uptime_ms;sequence;us;channel;level\n");
  for(uint8_t i=0;i<activeCount;++i){
    const uint8_t id=activeIds[i];const Channel &ch=channels[id];
    snprintf(meta,sizeof(meta),"# C;%u;%u;%.6f;%.6f;%.2f;%.2f;%.2f;%.3f\n",
      (unsigned)id,(unsigned)ch.type,(double)ch.gain,(double)ch.offset,
      (double)ch.rFixed,(double)ch.r25,(double)ch.beta,(double)ch.temperatureTrim);
    sdAppend(meta);
  }
  sdFlush();return true;
#else
  return false;
#endif
}
void sendSdStatus(WiFiClient &c){
  BufferedHttpPrint out(c);headers(out,"application/json");
  out.print(F("{\"compiled\":"));out.print(ODL_ENABLE_SD);
  out.print(F(",\"ready\":"));out.print(sdReady?1:0);
  out.print(F(",\"logging\":"));out.print(sdLogging?1:0);
  out.print(F(",\"file\":"));jsonText(out,sdFilename);
  out.print(F(",\"bytes\":"));out.print(sdBytesWritten);
  out.print(F(",\"errors\":"));out.print(sdWriteErrors);
  out.print(F(",\"missedEdges\":"));out.print(sdMissedEdges);
  out.println('}');out.finish();
}
void sendSdList(WiFiClient &c){
  BufferedHttpPrint out(c);headers(out,"application/json");
  out.print(F("{\"files\":["));
#if ODL_ENABLE_SD
  if(sdReady){
    File root=SD.open("/");
    bool first=true;
    if(root){
      while(true){
        File f=root.openNextFile();if(!f)break;
        String name=f.name();
        if(!f.isDirectory()&&name.startsWith("ODL")&&name.length()<=12){
          if(!first)out.print(',');
          first=false;
          jsonText(out,name.c_str());
        }
        f.close();
      }
      root.close();
    }
  }
#endif
  out.println(F("]}"));out.finish();
}
void sendSdDownload(WiFiClient &c,const String &url){
#if ODL_ENABLE_SD
  const String file=param(url,"name");
  if(!sdReady||file.length()!=11||!file.startsWith("ODL")||
     file.substring(7)!=".CSV"){
    sendHttpError(c,"400 Bad Request","Nome de sessao invalido");return;
  }
  for(int i=3;i<7;++i){if(file[i]<'0'||file[i]>'9'){
    sendHttpError(c,"400 Bad Request","Nome de sessao invalido");return;
  }}
  if(sdLogging && file==sdFilename){
    sendHttpError(c,"409 Conflict","Parar gravacao antes de descarregar esta sessao");return;
  }
  File source=SD.open(file.c_str(),FILE_READ);
  if(!source){sendHttpError(c,"404 Not Found","Sessao nao encontrada");return;}
  c.println(F("HTTP/1.1 200 OK"));
  c.println(F("Content-Type: text/csv; charset=utf-8"));
  c.print(F("Content-Disposition: attachment; filename=\""));c.print(file);c.println(F("\""));
  c.println(F("Connection: close"));c.println();
  uint8_t buffer[256];
  while(source.available()){
    const int n=source.read(buffer,sizeof(buffer));
    if(n<=0)break;
    c.write(buffer,(size_t)n);
    serviceBackgroundDuringPage();
  }
  source.close();
#else
  (void)url;sendHttpError(c,"501 Not Implemented","Compilar com ODL_ENABLE_SD=1");
#endif
}
