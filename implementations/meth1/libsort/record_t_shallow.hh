#ifndef RECORD_T_SHALLOW_HH
#define RECORD_T_SHALLOW_HH

#include <cstdint>
#include <cstring>
#include <iostream>
#include <utility>

#include "io_device.hh"

#include "alloc.hh"
#include "record_common.hh"
#include "record_ptr.hh"
#include "record_loc.hh"

/**
 * Version of Record that only performs a shallow copy on copy construction or
 * copy assignment. THIS IS DANGEROUS! It's very easy to end up with two
 * RecordS's that both think they own the same value memory, leading to
 * double-free's or use-after-free.
 *
 * However, we provide it as when used correctly it can be useful for some
 * performance sensitive situations.
 */
class RecordS
{
private:
#if WITHLOC == 1
  uint64_t loc_ = 0;
#endif
  uint8_t * val_ = nullptr;
  uint8_t key_[Rec::KEY_LEN];

public:
  void copy( const uint8_t * k, const uint8_t * v, uint64_t i ) noexcept
  {
#if WITHLOC == 1
    loc_ = i;
#else
    (void) i;
#endif
    if ( val_ == nullptr ) { val_ = Rec::alloc_val(); }
    memcpy( val_, v, Rec::VAL_LEN );
    memcpy( key_, k, Rec::KEY_LEN );
  }

  void copy( const uint8_t* r, uint64_t i ) noexcept
  {
    copy( r, r + Rec::KEY_LEN, i );
  }

  void copy( const RecordS & r ) noexcept
  {
    copy( r.key(), r.val(), r.loc() );
  }

  void copy( RecordS && r ) noexcept
  {
#if WITHLOC == 1
    loc_ = r.loc_;
#endif
    val_ = r.val_;
    memcpy( key_, r.key_, Rec::KEY_LEN );
  }

  void copy( const RecordPtr & r ) noexcept
  {
    copy( r.key(), r.val(), r.loc() );
  }

  RecordS( void ) noexcept {}

  /* Construct a min or max record. */
  explicit RecordS( Rec::limit_t lim ) noexcept
    : val_{Rec::alloc_val()}
  {
    if ( lim == Rec::MAX ) {
      memset( key_, 0xFF, Rec::KEY_LEN );
    } else {
      memset( key_, 0x00, Rec::KEY_LEN );
    }
  }

  /* Construct from c string read from disk */
  RecordS( const uint8_t * s, uint64_t loc = 0 ) { copy( s, loc ); }
  RecordS( const char * s, uint64_t loc = 0 ) { copy( (uint8_t *) s, loc ); }
  explicit RecordS( const RecordPtr & rptr )
  {
    copy( rptr.key(), rptr.val(), rptr.loc() );
  }
  RecordS( const RecordLoc &rloc, const uint8_t * v )
  {
      copy(rloc.key(), v, rloc.loc());
  }

  /* Copy constructor. WARNING: This only does a shallow copy! */
  RecordS( const RecordS & other )
#if WITHLOC == 1
    : loc_{other.loc_}
    , val_{other.val_}
#else
    : val_{other.val_}
#endif
  {
    memcpy( key_, other.key_, Rec::KEY_LEN );
  }

  /* Copy assignment. WARNING: This only does a shallow copy! */
  RecordS & operator=( const RecordS & other )
  {
    if ( this != &other ) {
#if WITHLOC == 1
      loc_ = other.loc_;
#endif
      val_ = other.val_;
      memcpy( key_, other.key_, Rec::KEY_LEN );
    }
    return *this;
  }

  RecordS( RecordS && other )
#if WITHLOC == 1
    : loc_{other.loc_}
#endif
  {
    val_ = other.val_;
    other.val_ = nullptr;
    memcpy( key_, other.key_, Rec::KEY_LEN );
  }

  RecordS & operator=( RecordS && other )
  {
    if ( this != &other ) {
#if WITHLOC == 1
      loc_ = other.loc_;
#endif
      // swap values
      uint8_t * v = val_;
      val_ = other.val_;
      other.val_ = v;
      memcpy( key_, other.key_, Rec::KEY_LEN );
    }
    return *this;
  }

  ~RecordS( void ) { Rec::dealloc_val( val_ ); }

  /* Accessors */
  const uint8_t * key( void ) const noexcept { return key_; }
  const uint8_t * val( void ) const noexcept { return val_; }
#if WITHLOC == 1
  uint64_t loc( void ) const noexcept { return loc_; }
#else
  uint64_t loc( void ) const noexcept { return 0; }
#endif

  /* Setters */
  void set_val( uint8_t * v ) noexcept { val_ = v; }

  /* methods for boost::sort */
  const char * data( void ) const noexcept { return (char *) key_; }
  unsigned char operator[]( size_t i ) const noexcept { return key_[i]; }
  size_t size( void ) const noexcept { return Rec::KEY_LEN; }

  /* comparison */
  comp_op( <, Record )
  comp_op( <, RecordS )
  comp_op( <, RecordPtr )
  comp_op( <=, Record )
  comp_op( <=, RecordS )
  comp_op( <=, RecordPtr )
  comp_op( >, Record )
  comp_op( >, RecordS )
  comp_op( >, RecordPtr )

  int compare( const uint8_t * k, uint64_t l ) const noexcept;
  int compare( const char * k, uint64_t l ) const noexcept;
  int compare( const Record & b ) const noexcept;
  int compare( const RecordS & b ) const noexcept;
  int compare( const RecordPtr & b ) const noexcept;

  /* Write to IO device */
  void write( IODevice & io, Rec::loc_t locinfo = Rec::NO_LOC ) const
  {
    io.write_all( (const char *) key_, Rec::KEY_LEN );
    io.write_all( (const char *) val_, Rec::VAL_LEN );
    if ( locinfo == Rec::WITH_LOC ) {
      uint64_t l = loc();
      io.write_all( (const char *) &l, Rec::LOC_LEN );
    }
  }
#if PACKED == 1
} __attribute__((packed));
#else
};
#endif

std::ostream & operator<<( std::ostream & o, const RecordS & r );

inline void iter_swap ( RecordS * a, RecordS * b ) noexcept
{
  std::swap( *a, *b );
}

#endif /* RECORD_T_SHALLOW_HH */
