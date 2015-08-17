#ifndef ALLOC_HH
#define ALLOC_HH

/**
 * Allocation critical for performance. Best results have been:
 * - boost::pool (null lock) -- 60ms
 * - tcmalloc                -- 135ms
 * - jemalloc                -- 180ms
 * - hoard                   -- 205ms
 * - malloc (glibc)          -- 245ms
 * - new                     -- 285ms
 * - boost::pool (mutex)     -- 540ms
 */
#define USE_MPOOL 1
#define USE_NEW   0

#include "record_common.hh"

#if USE_MPOOL == 1
#include <iostream>
#include "MemoryPool.h"
namespace Rec {
  using val_type = uint8_t[Rec::VAL_LEN];
  using Alloc = MemoryPool<val_type>;

  extern Alloc rec_pool;

  static inline uint8_t * alloc_val( void )
  {
    return (uint8_t *) Rec::rec_pool.allocate();
  }

  static inline void dealloc_val( uint8_t * v )
  {
    Rec::rec_pool.deallocate( (val_type *) v );
  }
}

#else /* USE_NEW */
namespace Rec {
  static inline uint8_t * alloc_val( void )
  {
    return new uint8_t[Rec::VAL_LEN];
  }

  static inline void dealloc_val( uint8_t * v )
  {
    if ( v != nullptr ) { delete v; }
  }
}
#endif

#endif /* ALLOC_HH */
