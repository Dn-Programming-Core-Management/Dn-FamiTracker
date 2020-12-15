#ifndef _DEVINFO_H_
#define _DEVINFO_H_
#include <math.h>
// デバイス情報
namespace xgm
{
  class IDeviceInfo
  {
  public:
    virtual IDeviceInfo *Clone()=0;
    virtual ~IDeviceInfo() {};
  };

  class ITrackInfo : public IDeviceInfo
  {
  public:
    virtual IDeviceInfo *Clone()=0;
    // 現在の出力値をそのまま返す
    virtual INT32 GetOutput()=0;
    // 周波数をHzで返す
    virtual double GetFreqHz()=0;
    // 周波数をデバイス依存値で返す．
    virtual UINT32 GetFreq()=0;
    // 音量を返す
    virtual INT32 GetVolume()=0;
    // 音量の最大値を返す
    virtual INT32 GetMaxVolume()=0;
    // 発音中ならtrue OFFならfalse
    virtual bool GetKeyStatus()=0;
    // トーン番号
    virtual INT32 GetTone()=0;

    // 周波数をノート番号に変換．0x60がo4c 0は無効
    static int GetNote(double freq)
    {
      const double LOG2_440 = 8.7813597135246596040696824762152;
      const double LOG_2 = 0.69314718055994530941723212145818;
      const int NOTE_440HZ = 0x69;

      if(freq>1.0)
        return (int)((12 * ( log(freq)/LOG_2 - LOG2_440 ) + NOTE_440HZ + 0.5));
      else
        return 0;
    }
  };

  /* TrackInfo を バッファリング */
  class InfoBuffer
  {
    int bufmax;
    int index;
    std::pair<int, IDeviceInfo*> *buffer;

  public:
    InfoBuffer(int max=60*10)
    {
      index = 0;
      bufmax = max;
      buffer = new std::pair<int, IDeviceInfo*>[bufmax];
      for(int i=0;i<bufmax;i++)
      {
        buffer[i].first = 0;
        buffer[i].second = NULL;
      }
    }
    virtual ~InfoBuffer()
    {
      for(int i=0;i<bufmax;i++)
        delete buffer[i].second;
      delete [] buffer;
    }

    virtual void Clear()
    {
      for(int i=0;i<bufmax;i++)
      {
        delete buffer[i].second;
        buffer[i].first = 0;
        buffer[i].second = NULL;
      }
    }

    virtual void AddInfo(int pos, IDeviceInfo *di)
    {
      if(di)
      {
        delete buffer[index].second;
        buffer[index].first=pos;
        buffer[index].second=di->Clone();
        index = (index+1)%bufmax;
      }
    }

    virtual IDeviceInfo *GetInfo(int pos)
    {
      if(pos==-1)
        return buffer[(index+bufmax-1)%bufmax].second;

      for(int i=(index+bufmax-1)%bufmax; i!=index; i=(i+bufmax-1)%bufmax)
        if(buffer[i].first<=pos) return buffer[i].second;
      return NULL;
    }
  };

  class TrackInfoBasic : public ITrackInfo
  {
  public:
    INT32 output;
    INT32 volume;
    INT32 max_volume;
    UINT32 _freq;
    double freq;
    bool key;
    INT32 tone;
    virtual IDeviceInfo *Clone(){ return new TrackInfoBasic(*this); }
    virtual INT32 GetOutput(){ return output; }
    virtual double GetFreqHz(){ return freq; }
    virtual UINT32 GetFreq(){ return _freq; }
    virtual bool GetKeyStatus(){ return key; }
    virtual INT32 GetVolume(){ return volume; }
    virtual INT32 GetMaxVolume(){ return max_volume; }
    virtual INT32 GetTone(){ return tone; };
  };

}// namespace xgm
#endif
