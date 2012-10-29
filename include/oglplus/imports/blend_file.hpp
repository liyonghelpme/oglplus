/**
 *  @file oglplus/imports/blend_file.hpp
 *  @brief Tools for reading Blender's .blend files
 *
 *  @author Matus Chochlik
 *
 *  Copyright 2010-2012 Matus Chochlik. Distributed under the Boost
 *  Software License, Version 1.0. (See accompanying file
 *  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef OGLPLUS_IMPORTS_BLEND_FILE_1107121519_HPP
#define OGLPLUS_IMPORTS_BLEND_FILE_1107121519_HPP

#include <oglplus/imports/blend_file/reader_client.hpp>
#include <oglplus/imports/blend_file/range.hpp>
#include <oglplus/imports/blend_file/info.hpp>
#include <oglplus/imports/blend_file/sdna.hpp>
#include <oglplus/imports/blend_file/block.hpp>
#include <oglplus/imports/blend_file/type.hpp>
#include <oglplus/imports/blend_file/structure.hpp>
#include <oglplus/imports/blend_file/flattened.hpp>
#include <oglplus/imports/blend_file/block_data.hpp>

namespace oglplus {
namespace imports {

class BlendFile
 : public BlendFileReaderClient
{
private:
	BlendFileReader _reader;

	BlendFileInfo _info;

	std::vector<BlendFileBlock> _blocks;
	std::map<uint64_t, std::size_t> _block_map;

	std::shared_ptr<BlendFileSDNA> _sdna;

public:
	BlendFile(std::istream& input)
	 : _reader(input)
	 , _info(_reader)
	{
		std::size_t block_idx = 0;
		while(!_eof(_reader))
		{
			std::array<char,4> code  = _read_array<4>(
				_reader,
				"Failed to read file block code"
			);
			if(_equal(code, "DNA1"))
			{
				_blocks.emplace_back(
					_reader,
					_info,
					std::move(code),
					false
				);
				_sdna = std::make_shared<BlendFileSDNA>(
					_reader,
					_info
				);
			}
			else
			{
				_blocks.emplace_back(
					_reader,
					_info,
					std::move(code),
					true
				);
			}
			_block_map[_blocks.back()._old_ptr] = block_idx++;
		}
		if(!_sdna)
		{
			throw std::runtime_error(
				"Blend file does not contain SDNA block"
			);
		}
	}

	const BlendFileInfo& Info(void) const
	{
		return _info;
	}

	BlendFileStructRange Structures(void) const
	{
		return BlendFileStructRange(_sdna);
	}

	BlendFileStruct BlockStructure(const BlendFileBlock& block) const
	{
		return BlendFileStruct(_sdna, block._sdna_index);
	}

	BlendFileBlockData BlockData(const BlendFileBlock& block)
	{
		std::vector<char> data;
		if(block.Size())
		{
			data.resize(block.Size());
			_go_to(_reader, block.DataPosition());
			_raw_read(
				_reader,
				data.data(),
				data.size(),
				"Failed to read blend file block data"
			);
		}
		return BlendFileBlockData(
			std::move(data),
			_info.ByteOrder(),
			_info.PointerSize()
		);
	}

	BlendFileBlockRange Blocks(void) const
	{
		return BlendFileBlockRange(_blocks);
	}
};

} // imports
} // oglplus

#endif // include guard