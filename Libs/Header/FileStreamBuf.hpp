#pragma once

#include "API/FileSystem.hpp"
#include <iostream>
#include <istream>
#include <ostream>
#include <streambuf>
#include <vector>
#include <memory>

namespace sdk::api::adapters
{

/**
 * @brief   Custom streambuf implementation that adapts sdk::api::File to std::iostream
 *
 * This class provides a bridge between the standard C++ iostream interface
 * and your custom file system API.
 */
class FileStreamBuf : public std::streambuf {
private:
    std::unique_ptr<sdk::api::File> file_;
    std::vector<char> input_buffer_;
    std::vector<char> output_buffer_;
    static const size_t buffer_size_ = 8192; // 8KB buffers
    bool is_write_mode_;
    bool is_open_;

    // Reading position (required for correct buffer handling)
    size_t read_position_;

public:
    /**
     * @brief   Constructor
     * @param   file: Unique pointer to your File implementation
     */
    explicit FileStreamBuf(std::unique_ptr<sdk::api::File> file)
        : file_(std::move(file))
        , input_buffer_(buffer_size_)
        , output_buffer_(buffer_size_)
        , is_write_mode_(false)
        , is_open_(false)
        , read_position_(0)
    {

        // Initializing buffers
        reset_input_buffer();
        reset_output_buffer();
    }

    /**
     * @brief   Destructor - ensures proper cleanup
     */
    ~FileStreamBuf()
    {
        close();
    }

    /**
     * @brief   Opens the file
     * @param   write_mode: true for write mode, false for read mode
     * @param   override_file: whether to override existing file
     * @retval  true if successful
     */
    bool open(bool write_mode = false, bool override_file = false)
    {
        if (is_open_) {
            close();
        }

        is_write_mode_ = write_mode;
        is_open_ = file_->open(write_mode, override_file);
        read_position_ = 0;

        if (is_open_) {
            reset_input_buffer();
            reset_output_buffer();
        }

        return is_open_;
    }

    /**
     * @brief   Checks if file is open
     */
    bool is_open() const
    {
        return is_open_ && file_->isOpen();
    }

    /**
     * @brief   Closes the file and flushes any pending writes
     */
    bool close()
    {
        bool success = true;

        if (is_open_) {
            // Flush any pending output
            success = (sync() == 0);

            // Close the underlying file
            if (!file_->close()) {
                success = false;
            }

            is_open_ = false;
        }

        return success;
    }

    /**
     * @brief   Gets the underlying File object
     */
    sdk::api::File* get_file() const
    {
        return file_.get();
    }

protected:
    /**
     * @brief   Reads data from file into input buffer when buffer is empty
     */
    int_type underflow() override
    {
        if (!is_open_) {
            return traits_type::eof();
        }

        // If there is data in the buffer, return it
        if (gptr() < egptr()) {
            return traits_type::to_int_type(*gptr());
        }

        // Reading new data from a file
        size_t bytes_read = 0;
        bool read_success = file_->read(input_buffer_.data(), buffer_size_, bytes_read);

        if (!read_success || bytes_read == 0) {
            return traits_type::eof();
        }

        // Updating the reading position
        read_position_ += bytes_read;

        // Setting buffer pointers
        setg(input_buffer_.data(),
            input_buffer_.data(),
            input_buffer_.data() + bytes_read);

        return traits_type::to_int_type(*gptr());
    }

    /**
     * @brief   Writes data to file when output buffer is full
     */
    int_type overflow(int_type c) override
    {
        if (!is_open_) {
            return traits_type::eof();
        }

        // Synchronize the buffer
        if (sync() != 0) {
            return traits_type::eof();
        }

        // If the character is not EOF, add it to the buffer
        if (c != traits_type::eof()) {
            *pptr() = traits_type::to_char_type(c);
            pbump(1);
        }

        return c;
    }

    /**
     * @brief   Synchronizes output buffer with file
     */
    int sync() override
    {
        if (!is_open_) {
            return -1;
        }

        size_t bytes_to_write = static_cast<size_t>(pptr() - pbase());
        if (bytes_to_write > 0) {
            size_t bytes_written = 0;
            bool write_success = file_->write(pbase(), bytes_to_write, bytes_written);

            if (!write_success || bytes_written != bytes_to_write) {
                return -1;
            }

            // Flush to physical media
            if (!file_->flush()) {
                return -1;
            }

            // Reset the output buffer
            reset_output_buffer();
        }

        return 0;
    }

    /**
     * @brief   Seeks to absolute position
     */
    std::streampos seekpos(std::streampos pos, std::ios_base::openmode mode) override
    {
        return seekoff(pos - std::streampos(0), std::ios_base::beg, mode);
    }

    /**
     * @brief   Seeks with offset
     */
    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir dir,
        std::ios_base::openmode mode) override
    {
        if (!is_open_) {
            return std::streampos(-1);
        }

        // Synchronize buffers before changing position
        if (sync() != 0) {
            return std::streampos(-1);
        }

        // Calculating a new position
        size_t new_pos = 0;
        size_t current_pos = file_->getPosition();

        switch (dir) {
        case std::ios_base::beg:
            if (off < 0) return std::streampos(-1);
            new_pos = static_cast<size_t>(off);
            break;

        case std::ios_base::cur: {
            std::streamoff signed_current = static_cast<std::streamoff>(current_pos);
            std::streamoff new_signed_pos = signed_current + off;
            if (new_signed_pos < 0) {
                return std::streampos(-1);
            }
            new_pos = static_cast<size_t>(new_signed_pos);
            break;
        }

        case std::ios_base::end: {
            size_t file_size = file_->size();
            std::streamoff signed_file_size = static_cast<std::streamoff>(file_size);
            std::streamoff new_signed_pos = signed_file_size + off;
            if (new_signed_pos < 0) {
                return std::streampos(-1);
            }
            new_pos = static_cast<size_t>(new_signed_pos);
            break;
        }

        default:
            return std::streampos(-1);
        }

        // Setting a new position
        if (!file_->seek(new_pos)) {
            return std::streampos(-1);
        }

        // Reset buffers after position change
        reset_input_buffer();
        reset_output_buffer();
        read_position_ = new_pos;

        return std::streampos(static_cast<std::streamoff>(new_pos));
    }

private:
    void reset_input_buffer()
    {
        setg(input_buffer_.data(),
            input_buffer_.data() + buffer_size_,
            input_buffer_.data() + buffer_size_);
    }

    void reset_output_buffer()
    {
        setp(output_buffer_.data(),
            output_buffer_.data() + buffer_size_);
    }
};

/**
 * @brief   File stream class that uses your File interface
 *
 * This class provides a std::iostream interface for your File objects,
 * allowing them to be used with any library expecting standard streams.
 */
class FileStream : public std::iostream {
private:
    std::unique_ptr<FileStreamBuf> stream_buf_;

public:
    /**
     * @brief   Constructor
     * @param   file: Unique pointer to your File implementation
     */
    explicit FileStream(std::unique_ptr<sdk::api::File> file)
        : std::iostream(nullptr)
    {

        stream_buf_ = std::make_unique<FileStreamBuf>(std::move(file));
        std::iostream::rdbuf(stream_buf_.get());
    }

    /**
     * @brief   Opens the file
     * @param   write_mode: true for write mode, false for read mode
     * @param   override_file: whether to override existing file
     * @retval  true if successful
     */
    bool open(bool write_mode = false, bool override_file = false)
    {
        bool success = stream_buf_->open(write_mode, override_file);
        if (success) {
            std::iostream::clear(); // Clearing stream error flags
        } else {
            std::iostream::setstate(std::ios_base::failbit);
        }
        return success;
    }

    /**
     * @brief   Closes the file
     */
    bool close()
    {
        bool success = stream_buf_->close();
        if (!success) {
            std::iostream::setstate(std::ios_base::failbit);
        }
        return success;
    }

    /**
     * @brief   Checks if file is open
     */
    bool is_open() const
    {
        return stream_buf_->is_open();
    }

    /**
     * @brief   Gets the underlying File object
     */
    sdk::api::File* get_file() const
    {
        return stream_buf_->get_file();
    }

    /**
     * @brief   Gets file size
     */
    size_t file_size() const
    {
        if (stream_buf_->get_file()) {
            return stream_buf_->get_file()->size();
        }
        return 0;
    }

    /**
     * @brief   Truncates file at current position
     */
    bool truncate()
    {
        if (!stream_buf_->is_open()) {
            return false;
        }

        // Synchronizing buffers
        if (std::iostream::sync() != 0) {
            return false;
        }

        size_t current_pos = stream_buf_->get_file()->getPosition();
        return stream_buf_->get_file()->truncate(current_pos);
    }

    /**
     * @brief   Truncates file at specified position
     */
    bool truncate(size_t offset)
    {
        if (!stream_buf_->is_open()) {
            return false;
        }

        // Synchronizing buffers
        if (std::iostream::sync() != 0) {
            return false;
        }

        return stream_buf_->get_file()->truncate(offset);
    }
};

/**
 * @brief   Factory function to create FileStream from FileSystem
 * @param   file_system: Pointer to your FileSystem implementation
 * @param   path: Path to the file
 * @retval  Unique pointer to FileStream
 */
inline std::unique_ptr<FileStream> make_file_stream(
    sdk::api::FileSystem* file_system,
    const char* path)
{

    auto file = file_system->file(path);
    if (!file) {
        return nullptr;
    }

    return std::make_unique<FileStream>(std::move(file));
}

} // namespace sdk::api::adapters

// Usage example:
/*

#include "your_filesystem_implementation.h" // Your FileSystem implementation

// Library function that expects std::iostream
void some_library_function(std::iostream& file)
{
    file << "Hello from library!" << std::endl;
    file << "Number: " << 42 << std::endl;

    // Reading back
    file.seekg(0, std::ios::beg);
    std::string line;
    while (std::getline(file, line)) {
        std::cout << "Read: " << line << std::endl;
    }
}

int main()
{
    // Creating your filesystem
    auto fs = std::make_unique<YourFileSystemImplementation>();

    // Creating an adapter
    auto file_stream = sdk::api::adapters::make_file_stream(fs.get(), "test.txt");

    if (file_stream && file_stream->open(true, true)) { // write mode, override
        std::cout << "File opened successfully" << std::endl;

        // Transfer to the library as a normal iostream
        some_library_function(*file_stream);

        file_stream->close();
        std::cout << "File closed" << std::endl;
    } else {
        std::cout << "Failed to open file" << std::endl;
    }

return 0;
}

*/