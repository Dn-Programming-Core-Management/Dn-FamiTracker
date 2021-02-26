#ifndef _DEVICE_H_
#define _DEVICE_H_
#include <stdio.h>
#include <vector>
#include <optional>
#include <assert.h>
#include "../xtypes.h"
#include "devinfo.h"
#include "../debugout.h"

namespace xgm
{
  const double DEFAULT_CLOCK = 1789772.0;
  const int DEFAULT_RATE = 48000;

  /**
   * エミュレータで使用するデバイスの抽象
   */
  class IDevice
  {
  public:
    /**
     * デバイスのリセット
     *
     * <P>
     * このメソッドの呼び出し後，このデバイスはどのようなメソッドの呼び
     * 出しに対しても，実行時エラーを起こしてはならない．逆に，このメソッ
     * ドを呼ぶ以前は，他のメソッドの動作は一切保証しなくても良い。
     * </P>
     */
    virtual void Reset () = 0;

    /**
     * デバイスへの書き込み
     * 
     * @param adr アドレス
     * @param val 書き込む値
     * @param id  デバイス識別情報．一つのデバイスが複数のIOをサポートする時など
     * @return 成功時 true 失敗時 false
     */
    virtual bool Write (UINT32 adr, UINT32 val, UINT32 id=0)=0;

    /**
     * デバイスから読み込み
     *
     * @param adr アドレス
     * @param val 読み出した値を受け取る変数．
     * @return 成功時 true 失敗時 false
     */
    virtual bool Read (UINT32 adr, UINT32 & val, UINT32 id=0)=0;

    /**
     * 各種オプションを設定する(もしあれば)
     */
    virtual void SetOption (int id, int val){};
    virtual ~IDevice() {};
  };

  /**
   * インターフェース：音声のレンダリングが可能なクラス
   */
  class IRenderable
  {
  public:
    /**
     * 音声のレンダリング
     * 
     * @param b[2] 合成されたデータを格納する配列．
     * b[0]が左チャンネル，b[1]が右チャンネルの音声データ．
     * @return 合成したデータのサイズ．1ならモノラル．2ならステレオ．0は合成失敗．
     */
    virtual UINT32 Render (INT32 b[2]) = 0;

    // When seeking, this replaces Render
    virtual void Skip () {}

    /**
     *  chip update/operation is now bound to CPU clocks
     *  Render() now simply mixes and outputs sound
     */
    virtual void Tick (UINT32 clocks) {}
    virtual ~IRenderable() {};
  };

  /**
   * 音声合成チップ
   */
  class ISoundChip : public IDevice, virtual public IRenderable
  {
  public:
    /**
     * Soundchip clocked by M2 (NTSC = ~1.789MHz)
     */
    virtual void Tick (UINT32 clocks) = 0;

    /**
     * This interface only allows you to advance time (Tick)
     * and poll for the latest amplitude (Render).
     * If you call Tick() with an argument greater than 1, you lose precision,
     * and with a small argument, you burn more CPU time.
     *
     * As an optimization,
     * this function returns the number of clocks before the next level change.
     * If the chip does not know how to compute it, it returns a placeholder value.
     */
    virtual UINT32 ClocksUntilLevelChange () {
        constexpr int NSFPLAY_RENDER_STEP = 4;
        return NSFPLAY_RENDER_STEP;
    }

    /**
     * チップの動作クロックを設定
     *
     * @param clock 動作周波数
     */
    virtual void SetClock (double clock) = 0;

    /**
     * 音声合成レート設定
     *
     * @param rate 出力周波数
     */
    virtual void SetRate (double rate) = 0;

    /**
     * Channel mask.
     */
    virtual void SetMask (int mask)=0;

    /**
     * Stereo mix.
     *   mixl = 0-256
     *   mixr = 0-256
     *     128 = neutral
     *     256 = double
     *     0 = nil
     *    <0 = inverted
     */
    virtual void SetStereoMix(int trk, xgm::INT16 mixl, xgm::INT16 mixr) = 0;

    /**
     * Track info for keyboard view.
     */
    virtual ITrackInfo *GetTrackInfo(int trk){ return NULL; }
    virtual ~ISoundChip() {};
  };

  /**
   * バス
   *
   * <P>
   * 複数のデバイスに，リセット，書き込み，読み込み動作をブロードキャストする．
   * <P>
   */
  class Bus : public IDevice
  {
  protected:
    std::vector < IDevice * > vd;
  public:
    /**
     * リセット
     *
     * <P>
     * 取り付けられている全てのデバイスの，Resetメソッドを呼び出す．
     * 呼び出し順序は，デバイスが取り付けられた順序に等しい．
     * </P>
     */
    void Reset ()
    {
      std::vector < IDevice * >::iterator it;
      for (it = vd.begin (); it != vd.end (); it++)
        (*it)->Reset ();
    }

    /**
     * 全デバイスの取り外し
     */
    void DetachAll ()
    {
      vd.clear ();
    }

    /**
     * デバイスの取り付け
     *
     * <P>
     * このバスにデバイスを取り付ける．
     * </P>
     *
     * @param d 取り付けるデバイスへのポインタ
     */
    void Attach (IDevice * d)
    {
      vd.push_back (d);
    }

    /**
     * 書き込み
     *
     * <P>
     * 取り付けられている全てのデバイスの，Writeメソッドを呼び出す．
     * 呼び出し順序は，デバイスが取り付けられた順序に等しい．
     * </P>
     */
    bool Write (UINT32 adr, UINT32 val, UINT32 id=0)
    {
      bool ret = false;
      std::vector < IDevice * >::iterator it;
      for (it = vd.begin (); it != vd.end (); it++)
        ret |= (*it)->Write (adr, val);
      return ret;
    }

    /**
     * 読み込み
     *
     * <P>
     * 取り付けられている全てのデバイスのReadメソッドを呼び出す．
     * 呼び出し順序は，デバイスが取り付けられた順序に等しい．
     * 帰り値は有効な(Readメソッドがtrueを返却した)デバイスの
     * 返り値の論理和．
     * </P>
     */
    bool Read (UINT32 adr, UINT32 & val, UINT32 id=0)
    {
      bool ret = false;
      UINT32 vtmp = 0;
      std::vector < IDevice * >::iterator it;

      val = 0;
      for (it = vd.begin (); it != vd.end (); it++)
      {
        if ((*it)->Read (adr, vtmp))
        {
          val |= vtmp;
          ret = true;
        }
      }
      return ret;
    }
  };

  /**
   * レイヤー
   *
   * <P>
   * バスと似ているが，読み書きの動作を全デバイスに伝播させない．
   * 最初に読み書きに成功したデバイスを発見した時点で終了する．
   * </P>
   */
  class Layer : public Bus
  {
  protected:
  public:
    /**
     * 書き込み
     *
     * <P>
     * 取り付けられているデバイスのWriteメソッドを呼び出す．
     * 呼び出し順序は，デバイスが取り付けられた順序に等しい．
     * Writeに成功したデバイスが見つかった時点で終了．
     * </P>
     */
    bool Write (UINT32 adr, UINT32 val, UINT32 id=0)
    {
      std::vector < IDevice * >::iterator it;
      for (it = vd.begin (); it != vd.end (); it++)
        if ((*it)->Write (adr, val))
          return true;
      return false;
    }

    /**
     * 読み込み
     *
     * <P>
     * 取り付けられているデバイスのReadメソッドを呼び出す．
     * 呼び出し順序は，デバイスが取り付けられた順序に等しい．
     * Readに成功したデバイスが見つかった時点で終了．
     * </P>
     */
    bool Read (UINT32 adr, UINT32 & val, UINT32 id=0)
    {
      std::vector < IDevice * >::iterator it;
      val = 0;
      for (it = vd.begin (); it != vd.end (); it++)
        if ((*it)->Read (adr, val))
          return true;
      return false;
    }
  };

}

#endif
