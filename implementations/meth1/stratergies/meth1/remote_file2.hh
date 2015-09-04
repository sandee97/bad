#ifndef REMOTE_FILE2_HH
#define REMOTE_FILE2_HH

#include <exception>
#include <memory>

#include "circular_io_rec.hh"
#include "record.hh"

#include "client.hh"

/**
 * Strategy 1.
 * - No upfront work.
 * - Full linear scan for each record.
 */
namespace meth1
{

/**
 * RemoteFile2 is a helper wrapper around a Client presents a stateful File
 * interface and performs read-ahead.
 */
class RemoteFile2
{
private:
  Client * c_;
  CircularIORec<Rec::SIZE> * buf_;

  uint64_t chunkSize_;
  uint64_t bufSize_;

  bool start_;
  uint64_t ioPos_;
  uint64_t pos_;
  uint64_t size_;

  RecordPtr head_;

public:
  RemoteFile2( Client & c, uint64_t chunkSize, uint64_t bufSize )
    : c_{&c}
    , buf_{new CircularIORec<Rec::SIZE>( c.socket(), (uint64_t) c.socket().fd_num() )}
    , chunkSize_{chunkSize}
    , bufSize_{bufSize}
    , start_{true}
    , ioPos_{0}
    , pos_{0}
    , size_{0}
    , head_{(const char *) nullptr}
  {
    buf_->set_io_drained_cb( [this]() {
      std::cout << "io callback, " << this->c_->socket().fd_num()
        << ", " << this->ioPos_ << std::endl;
      this->nextChunk();
    } );
  }

  /* no copy */
  RemoteFile2( const RemoteFile2 & other ) = delete;
  RemoteFile2 & operator=( RemoteFile2 & other ) = delete;

  /* no move */
  RemoteFile2( RemoteFile2 && other ) = delete;
  RemoteFile2 & operator=( RemoteFile2 && other ) = delete;

  ~RemoteFile2( void )
  {
    if ( buf_ != nullptr ) { delete buf_; }
  }

  void sendSize( void ) { c_->sendSize(); }
  uint64_t recvSize( void ) { size_ = c_->recvSize(); return size_; }

  void nextChunk( void )
  {
    std::cout << "nextChunk: " << c_->socket().fd_num() << ", " << ioPos_ << std::endl;
    if ( size_ == 0 ) {
      sendSize();
      recvSize();
    }
    if ( ioPos_ < size_ ) {
      uint64_t siz = std::min( chunkSize_, size_ - ioPos_ );
      c_->sendRead( ioPos_, siz );
      std::cout << "sendRead, " << c_->socket().fd_num() << ", " << ioPos_
        << ", " << siz << std::endl;
      ioPos_ += siz;
    }
  }

  void nextRecord( void )
  {
    if ( eof() ) {
      return;
    }

    const char * recStr = nullptr;
    if ( start_ ) {
      recStr = nullptr;
      start_ = false;
    } else {
      recStr = buf_->next_record();
    }

    if ( recStr == nullptr ) {
      uint64_t nrecs = c_->recvRead();
      buf_->start_read( nrecs * Rec::SIZE );
      recStr = buf_->next_record();
    }

    head_ = {recStr};
    pos_++;

    if ( pos_ == size_ ) {
      // drain the final nullptr
      recStr = buf_->next_record();
      if ( recStr != nullptr ) {
        throw new std::runtime_error( "not actually eof" );
      }
    }
  }

  RecordPtr curRecord( void ) const noexcept { return head_; }
  bool eof( void ) const noexcept
  {
    return pos_ >= size_;
  }

  bool operator>( const RemoteFile2 & b ) const noexcept
  {
    return head_ > b.head_;
  }
};

/* Pointer wrapper around a RemoteFile2. */
class RemoteFilePtr2
{
private:
  RemoteFile2 * rf_;

public:
  RemoteFilePtr2( RemoteFile2 * rf )
    : rf_{rf}
  {}

  RemoteFile2 * rf( void ) const noexcept { return rf_; }

  RecordPtr curRecord( void ) const noexcept { return rf_->curRecord(); }
  void nextRecord( void ) { return rf_->nextRecord(); }
  bool eof( void ) const noexcept { return rf_->eof(); }

  bool operator>( const RemoteFilePtr2 & b ) const noexcept
  {
    return *rf_ > *b.rf_;
  }
};
}

#endif /* REMOTE_FILE2_HH */
