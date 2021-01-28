// Band-limited sound synthesis buffer

// Blip_Buffer 0.4.1
#ifndef BLIP_BUFFER_H
#define BLIP_BUFFER_H

// based off Q_DISABLE_COPY and the like.

#define BLIP_DISABLE_COPY(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;

#define BLIP_DISABLE_MOVE(Class) \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;

#define BLIP_DISABLE_COPY_MOVE(Class) \
    BLIP_DISABLE_COPY(Class) \
    BLIP_DISABLE_MOVE(Class)

    // internal
    #include <limits.h>
    #if INT_MAX < 0x7FFFFFFF
        #error "int must be at least 32 bits"
    #endif

    typedef int blip_long;
    typedef unsigned blip_ulong;

// Time unit at source clock rate
typedef blip_ulong blip_nclock_t;

// Output samples are 16-bit signed, with a range of -32768 to 32767
typedef blip_ulong blip_nsamp_t;
typedef short blip_amplitude_t;
enum { blip_sample_max = 32767 };

class Blip_Buffer {
public:
    typedef const char* blargg_err_t;

    // Set output sample rate and buffer length in milliseconds (1/1000 sec, defaults
    // to 1/4 second), then clear buffer. Returns NULL on success, otherwise if there
    // isn't enough memory, returns error without affecting current buffer setup.
    blargg_err_t set_sample_rate( blip_ulong samples_per_sec, blip_ulong msec_length = 1000 / 4 );

    // Set number of source time units per second
    void clock_rate( blip_ulong );

    // End current time frame of specified duration and make its samples available
    // (along with any still-unread samples) for reading with read_samples(). Begins
    // a new time frame at the end of the current frame.
    void end_frame( blip_nclock_t time );

    // Read at most 'max_samples' out of buffer into 'dest', removing them from from
    // the buffer. Returns number of samples actually read and removed. If stereo is
    // true, increments 'dest' one extra time after writing each sample, to allow
    // easy interleving of two channels into a stereo output buffer.
    blip_nsamp_t read_samples( blip_amplitude_t* dest, blip_nsamp_t max_samples, int stereo = 0 );

// Additional optional features

    // Current output sample rate
    blip_ulong sample_rate() const;

    // Length of buffer, in milliseconds
    blip_ulong length() const;

    // Number of source time units per second
    blip_ulong clock_rate() const;

    // Set frequency high-pass filter frequency, where higher values reduce bass more
    void bass_freq( blip_ulong frequency );

    // Number of samples delay from synthesis to samples read out
    int output_latency() const;

    // Remove all available samples and clear buffer to silence. If 'entire_buffer' is
    // false, just clears out any samples waiting rather than the entire buffer.
    void clear( int entire_buffer = 1 );

    // Number of samples available for reading with read_samples()
    blip_nsamp_t samples_avail() const;

    // Remove 'count' samples from those waiting to be read
    void remove_samples( blip_nsamp_t count );

// Experimental features

    // Count number of clocks needed until 'count' samples will be available.
    // If buffer can't even hold 'count' samples, returns number of clocks until
    // buffer becomes full.
    blip_nclock_t count_clocks( blip_nsamp_t nsamp ) const;

    // Number of raw samples that can be mixed within frame of specified duration.
    blip_nsamp_t count_samples( blip_nclock_t nclock ) const;

    // Mix 'count' samples from 'buf' into buffer.
    void mix_samples( blip_amplitude_t const* buf, blip_nsamp_t count );

    // not documented yet
    void set_modified() { modified_ = 1; }
    int clear_modified() { int b = modified_; modified_ = 0; return b; }
    typedef blip_ulong blip_resampled_time_t;
    void remove_silence( blip_nsamp_t count );
    blip_resampled_time_t resampled_duration( blip_ulong t ) const     { return t * factor_; }
    blip_resampled_time_t resampled_time( blip_nclock_t t ) const { return t * factor_ + offset_; }
    blip_resampled_time_t clock_rate_factor( blip_ulong clock_rate ) const;

public:
    Blip_Buffer();
    Blip_Buffer(blip_ulong smp_per_s, blip_ulong clk_per_s);
    ~Blip_Buffer();

    Blip_Buffer(Blip_Buffer &&);

    // Deprecated
    typedef blip_resampled_time_t resampled_time_t;
    blargg_err_t sample_rate( blip_ulong r ) { return set_sample_rate( r ); }
    blargg_err_t sample_rate( blip_ulong r, blip_ulong msec ) { return set_sample_rate( r, msec ); }
private:
    // noncopyable
    Blip_Buffer( const Blip_Buffer& );
    Blip_Buffer& operator = ( const Blip_Buffer& );
public:
    typedef blip_long buf_t_;
    blip_ulong factor_;
    blip_resampled_time_t offset_;
    buf_t_* buffer_;
    blip_nsamp_t buffer_size_;
    blip_long reader_accum_;
    int bass_shift_;
private:
    blip_ulong sample_rate_;
    blip_ulong clock_rate_;
    blip_ulong bass_freq_;
    blip_ulong length_;
    int modified_;
    friend class Blip_Reader;
};

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

// Number of bits in resample ratio fraction. Higher values give a more accurate ratio
// but reduce maximum buffer size.
#ifndef BLIP_BUFFER_ACCURACY
    #define BLIP_BUFFER_ACCURACY 16
#endif

// Number bits in phase offset.
// Fewer than 10 bits (1024 phase offsets) results in
// aliasing visible on Audacity's spectrum viewer,
// when synthesizing high-frequency NES triangle or VRC6 pulse waves.
// Affects size of Blip_Synth objects since they store the waveform directly.
#define BLIP_PHASE_BITS 10

    // Internal
    typedef blip_ulong blip_resampled_time_t;
    int const blip_widest_impulse_ = 16;
    int const blip_buffer_extra_ = blip_widest_impulse_ + 2;
    int const blip_res = 1 << BLIP_PHASE_BITS;
    class blip_eq_t;

    class Blip_Synth_Fast_ {
    public:
        int last_amp = 0;
        int delta_factor;

        void volume_unit( double );
        Blip_Synth_Fast_();
        void treble_eq( blip_eq_t const& ) { }
    };

    class Blip_Synth_ {
    public:
        int last_amp = 0;
        int delta_factor;

        void volume_unit( double );
        Blip_Synth_( short* impulses, int width );
        void treble_eq( blip_eq_t const& );
    private:
        double volume_unit_;
        short* const impulses;
        int const width;
        blip_long kernel_unit;
        int impulses_size() const { return blip_res / 2 * width + 1; }
        void adjust_impulse();
    };

// Quality level. Start with blip_good_quality.
const int blip_med_quality  = 8;
const int blip_good_quality = 12;
const int blip_high_quality = 16;

// Range specifies the greatest expected change in amplitude. Calculate it
// by finding the difference between the maximum and minimum expected
// amplitudes (max - min).
template<int quality>
class Blip_Synth {
public:
    // Set overall volume of waveform
    // The actual output value (assuming no DC removal) is around
    // (amplitude / range) * volume * 65536.
    void volume( double v, unsigned int range ) { impl.volume_unit( v / range ); }

    // Configure low-pass filter (see blip_buffer.txt)
    void treble_eq( blip_eq_t const& eq )       { impl.treble_eq( eq ); }

    void clear() { impl.last_amp = 0; }
    // Update amplitude of waveform at given time. Using this requires a separate
    // Blip_Synth for each waveform.
    // The actual output value (assuming no DC removal) is around
    // (amplitude / range) * volume * 65536.
    void update( blip_nclock_t time, int amplitude, Blip_Buffer* );

// Low-level interface

    // Add an amplitude transition of specified delta, optionally into specified buffer
    // rather than the one set with output(). Delta can be positive or negative.
    // The actual change in amplitude is around (delta / range) * volume * 65536.
    void offset( blip_nclock_t, int delta, Blip_Buffer* ) const;

    // Works directly in terms of fractional output samples. Contact author for more info.
    void offset_resampled( blip_resampled_time_t, int delta, Blip_Buffer* ) const;

    // Same as offset(), except code is inlined for higher performance
    void offset_inline( blip_nclock_t t, int delta, Blip_Buffer* buf ) const {
        offset_resampled( t * buf->factor_ + buf->offset_, delta, buf );
    }

private:
#if BLIP_BUFFER_FAST
    Blip_Synth_Fast_ impl;
#else
    Blip_Synth_ impl;
    typedef short imp_t;
    imp_t impulses [blip_res * (quality / 2) + 1];
public:
    Blip_Synth() : impl(impulses, quality) {}

    // When update(...Amplitude) is called,
    // the actual output value (assuming no DC removal) is around
    // (Amplitude / range) * volume * 65536.
    Blip_Synth(unsigned int range, double volume) : impl( impulses, quality ) {
        this->volume(volume, range);
    }
    // Cannot be moved or copied because this struct is self-referencing:
    // Blip_Synth.(Blip_Synth_ impl).impulses points to Blip_Synth.impulses.
    BLIP_DISABLE_COPY_MOVE(Blip_Synth)
#endif
};

// Low-pass equalization parameters
class blip_eq_t {
public:
    // Logarithmic rolloff to treble dB at half sampling rate. Negative values reduce
    // treble, small positive values (0 to 5.0) increase treble.
    blip_eq_t( double treble_db = 0 );

    // See blip_buffer.txt
    blip_eq_t( double treble, blip_long rolloff_freq, blip_long sample_rate, blip_long cutoff_freq = 0 );

public:
    double treble;
    blip_long rolloff_freq;
    blip_long sample_rate;
    blip_long cutoff_freq;

private:
    void generate( float* out, int count ) const;
    friend class Blip_Synth_;
};

int const blip_sample_bits = 30;

// Dummy Blip_Buffer to direct sound output to, for easy muting without
// having to stop sound code.
class Silent_Blip_Buffer : public Blip_Buffer {
    buf_t_ buf [blip_buffer_extra_ + 1];
public:
    // The following cannot be used (an assertion will fail if attempted):
    blargg_err_t set_sample_rate( blip_long samples_per_sec, int msec_length );
    blip_nclock_t count_clocks( blip_long count ) const;
    void mix_samples( blip_amplitude_t const* buf, blip_long count );

    Silent_Blip_Buffer();
};

    #if defined (__GNUC__) || _MSC_VER >= 1100
        #define BLIP_RESTRICT __restrict
    #else
        #define BLIP_RESTRICT
    #endif

// Optimized reading from Blip_Buffer, for use in custom sample output

// Begin reading from buffer. Name should be unique to the current block.
#define BLIP_READER_BEGIN( name, blip_buffer ) \
    const Blip_Buffer::buf_t_* BLIP_RESTRICT name##_reader_buf = (blip_buffer).buffer_;\
    blip_long name##_reader_accum = (blip_buffer).reader_accum_

// Get value to pass to BLIP_READER_NEXT()
#define BLIP_READER_BASS( blip_buffer ) ((blip_buffer).bass_shift_)

// Constant value to use instead of BLIP_READER_BASS(), for slightly more optimal
// code at the cost of having no bass control
int const blip_reader_default_bass = 9;

// Current sample
#define BLIP_READER_READ( name )        (name##_reader_accum >> (blip_sample_bits - 16))

// Current raw sample in full internal resolution
#define BLIP_READER_READ_RAW( name )    (name##_reader_accum)

// Advance to next sample
#define BLIP_READER_NEXT( name, bass ) \
    (void) (name##_reader_accum += *name##_reader_buf++ - (name##_reader_accum >> (bass)))

// End reading samples from buffer. The number of samples read must now be removed
// using Blip_Buffer::remove_samples().
#define BLIP_READER_END( name, blip_buffer ) \
    (void) ((blip_buffer).reader_accum_ = name##_reader_accum)


// Compatibility with older version
const blip_long blip_unscaled = 65535;
const int blip_low_quality  = blip_med_quality;
const int blip_best_quality = blip_high_quality;

// Deprecated; use BLIP_READER macros as follows:
// Blip_Reader r; r.begin( buf ); -> BLIP_READER_BEGIN( r, buf );
// int bass = r.begin( buf )      -> BLIP_READER_BEGIN( r, buf ); int bass = BLIP_READER_BASS( buf );
// r.read()                       -> BLIP_READER_READ( r )
// r.read_raw()                   -> BLIP_READER_READ_RAW( r )
// r.next( bass )                 -> BLIP_READER_NEXT( r, bass )
// r.next()                       -> BLIP_READER_NEXT( r, blip_reader_default_bass )
// r.end( buf )                   -> BLIP_READER_END( r, buf )
class Blip_Reader {
public:
    int begin( Blip_Buffer& );
    blip_long read() const          { return accum >> (blip_sample_bits - 16); }
    blip_long read_raw() const      { return accum; }
    void next( int bass_shift = 9 )         { accum += *buf++ - (accum >> bass_shift); }
    void end( Blip_Buffer& b )              { b.reader_accum_ = accum; }

private:
    const Blip_Buffer::buf_t_* buf;
    blip_long accum;
};

// End of public interface

#include <assert.h>

template<int quality>
inline void Blip_Synth<quality>::offset_resampled( blip_resampled_time_t time,
        int delta, Blip_Buffer* blip_buf ) const
{
    // Fails if time is beyond end of Blip_Buffer, due to a bug in caller code or the
    // need for a longer buffer as set by set_sample_rate().
    assert( (blip_long) (time >> BLIP_BUFFER_ACCURACY) < blip_buf->buffer_size_ );
    delta *= impl.delta_factor;
    blip_long* BLIP_RESTRICT buf = blip_buf->buffer_ + (time >> BLIP_BUFFER_ACCURACY);
    int phase = (int) (time >> (BLIP_BUFFER_ACCURACY - BLIP_PHASE_BITS) & (blip_res - 1));

#if BLIP_BUFFER_FAST
    blip_long left = buf [0] + delta;

    // Kind of crappy, but doing shift after multiply results in overflow.
    // Alternate way of delaying multiply by delta_factor results in worse
    // sub-sample resolution.
    blip_long right = (delta >> BLIP_PHASE_BITS) * phase;
    left  -= right;
    right += buf [1];

    buf [0] = left;
    buf [1] = right;
#else

    int const fwd = (blip_widest_impulse_ - quality) / 2;
    int const rev = fwd + quality - 2;
    int const mid = quality / 2 - 1;

    imp_t const* BLIP_RESTRICT imp = impulses + blip_res - phase;

    #if defined (_M_IX86) || defined (_M_IA64) || defined (__i486__) || \
            defined (__x86_64__) || defined (__ia64__) || defined (__i386__)

    // straight forward implementation resulted in better code on GCC for x86

    #define ADD_IMP( out, in ) \
        buf [out] += (blip_long) imp [blip_res * (in)] * delta

    #define BLIP_FWD( i ) {\
        ADD_IMP( fwd     + i, i     );\
        ADD_IMP( fwd + 1 + i, i + 1 );\
    }
    #define BLIP_REV( r ) {\
        ADD_IMP( rev     - r, r + 1 );\
        ADD_IMP( rev + 1 - r, r     );\
    }

        BLIP_FWD( 0 )
        if ( quality > 8  ) BLIP_FWD( 2 )
        if ( quality > 12 ) BLIP_FWD( 4 )
        {
            ADD_IMP( fwd + mid - 1, mid - 1 );
            ADD_IMP( fwd + mid    , mid     );
            imp = impulses + phase;
        }
        if ( quality > 12 ) BLIP_REV( 6 )
        if ( quality > 8  ) BLIP_REV( 4 )
        BLIP_REV( 2 )

        ADD_IMP( rev    , 1 );
        ADD_IMP( rev + 1, 0 );

    #else

    // for RISC processors, help compiler by reading ahead of writes

    #define BLIP_FWD( i ) {\
        blip_long t0 =                       i0 * delta + buf [fwd     + i];\
        blip_long t1 = imp [blip_res * (i + 1)] * delta + buf [fwd + 1 + i];\
        i0 =           imp [blip_res * (i + 2)];\
        buf [fwd     + i] = t0;\
        buf [fwd + 1 + i] = t1;\
    }
    #define BLIP_REV( r ) {\
        blip_long t0 =                 i0 * delta + buf [rev     - r];\
        blip_long t1 = imp [blip_res * r] * delta + buf [rev + 1 - r];\
        i0 =           imp [blip_res * (r - 1)];\
        buf [rev     - r] = t0;\
        buf [rev + 1 - r] = t1;\
    }

        blip_long i0 = *imp;
        BLIP_FWD( 0 )
        if ( quality > 8  ) BLIP_FWD( 2 )
        if ( quality > 12 ) BLIP_FWD( 4 )
        {
            blip_long t0 =                   i0 * delta + buf [fwd + mid - 1];
            blip_long t1 = imp [blip_res * mid] * delta + buf [fwd + mid    ];
            imp = impulses + phase;
            i0 = imp [blip_res * mid];
            buf [fwd + mid - 1] = t0;
            buf [fwd + mid    ] = t1;
        }
        if ( quality > 12 ) BLIP_REV( 6 )
        if ( quality > 8  ) BLIP_REV( 4 )
        BLIP_REV( 2 )

        blip_long t0 =   i0 * delta + buf [rev    ];
        blip_long t1 = *imp * delta + buf [rev + 1];
        buf [rev    ] = t0;
        buf [rev + 1] = t1;
    #endif

#endif
}

#undef BLIP_FWD
#undef BLIP_REV

template<int quality>
#if BLIP_BUFFER_FAST
    inline
#endif
void Blip_Synth<quality>::offset( blip_nclock_t t, int delta, Blip_Buffer* buf ) const
{
    offset_resampled( t * buf->factor_ + buf->offset_, delta, buf );
}

template<int quality>
#if BLIP_BUFFER_FAST
    inline
#endif
void Blip_Synth<quality>::update( blip_nclock_t t, int amp, Blip_Buffer* blip_buf )
{
    int delta = amp - impl.last_amp;

    // nsfplay generates a new amplitude every clock.
    // Usually it's the same as the previous one.
    // If so, skip the slow synthesis process.
    if (delta != 0) {
        impl.last_amp = amp;
        offset_resampled( t * blip_buf->factor_ + blip_buf->offset_, delta, blip_buf);
    }
}

inline blip_eq_t::blip_eq_t( double t ) :
        treble( t ), rolloff_freq( 0 ), sample_rate( 44100 ), cutoff_freq( 0 ) { }
inline blip_eq_t::blip_eq_t( double t, blip_long rf, blip_long sr, blip_long cf ) :
        treble( t ), rolloff_freq( rf ), sample_rate( sr ), cutoff_freq( cf ) { }

inline blip_ulong  Blip_Buffer::length() const         { return length_; }
inline blip_nsamp_t Blip_Buffer::samples_avail() const  { return (blip_nsamp_t) (offset_ >> BLIP_BUFFER_ACCURACY); }
inline blip_ulong Blip_Buffer::sample_rate() const    { return sample_rate_; }
inline int  Blip_Buffer::output_latency() const { return blip_widest_impulse_ / 2; }
inline blip_ulong Blip_Buffer::clock_rate() const     { return clock_rate_; }
inline void Blip_Buffer::clock_rate( blip_ulong cps ) { factor_ = clock_rate_factor( clock_rate_ = cps ); }

inline int Blip_Reader::begin( Blip_Buffer& blip_buf )
{
    buf = blip_buf.buffer_;
    accum = blip_buf.reader_accum_;
    return blip_buf.bass_shift_;
}

int const blip_max_length = 0;
int const blip_default_length = 250;

#endif
