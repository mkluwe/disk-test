// Licensed under the MIT license. See the LICENSE file for more information.

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

std::filesystem::path get_path( std::string_view s ) {
    std::filesystem::path path( s );
    if ( !exists( path ) ) {
        throw std::runtime_error( "Cannot find path." );
    }
    if ( !is_directory( path ) && !is_regular_file( path ) ) {
        throw std::runtime_error( "Not a file or directory." );
    }
    if ( is_regular_file( path ) ) {
        path = path.parent_path();
    }
    return path;
}

int64_t available_space( const std::filesystem::path& path ) {
    return space( path ).available;
}

const int64_t headroom = 5000;
const int64_t chunk_size = 1000000000;

int64_t write_file( const std::filesystem::path& path, int64_t pos ) {
    auto size = std::min( available_space( path ) - headroom, chunk_size ) / 8;
    if ( size <= 0 ) {
        return 0;
    }
    std::ofstream of( path / std::to_string( pos ) );
    for ( const auto end = pos + size; pos < end; ++pos ) {
        of.write( reinterpret_cast< char* >( &pos ), sizeof pos );
    }
    return pos;
}

struct Parameter {
    Parameter( char *argv[], int argc ) {
        std::vector< std::string_view > args;
        for ( int i = 1; i < argc; ++i ) {
            args.push_back( argv[ i ] );
        }
        auto arg = args.begin();
        while ( arg != args.end() ) {
            if ( *arg == "-p" ) {
                ++arg;
                if ( arg == args.end() ) {
                    throw std::runtime_error( "No Path" );
                }
                path = get_path( *arg++ );
            } else if ( *arg == "-w" ) {
                ++arg;
                write = true;
            } else if ( *arg == "-c" ) {
                ++arg;
                check = true;
            } else {
                throw std::runtime_error( "Unknown or missing argument" );
            }
        }
        if ( path == "" ) {
            throw std::runtime_error( "No path given." );
        }
    }
    std::string path;
    bool write = false;
    bool check = false;
};

void write_files( const std::filesystem::path& path ) {
    for ( int64_t pos = 0; ( pos = write_file( path, pos ) ); );
}

void check_file( const std::filesystem::path& path ) {
    std::istringstream is( path.filename() );
    int64_t pos;
    is >> pos;
    std::ifstream ifs( path );
    int64_t input;
    while ( ifs.read( reinterpret_cast< char* >( &input ), sizeof input ) ) {
        if ( input != pos++ ) {
            throw std::runtime_error( "Read Error." );
        }
    }
}

void check_files( const std::filesystem::path& path ) {
    for ( auto& item: std::filesystem::directory_iterator( path ) ) {
        if ( item.is_regular_file() ) {
            check_file( item.path() );
        }
    }
}

int main( int argc, char* argv[] ) {
    Parameter parm( argv, argc );
    if ( parm.write ) {
        write_files( parm.path );
    }
    if ( parm.check ) {
        check_files( parm.path );
    }
}
