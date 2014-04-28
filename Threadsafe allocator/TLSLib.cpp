#include "TLSLib.hpp"

namespace tls
{
	size_t main_block_pool::bin_sizes[] = { 8, 16, 32, 64, 128, 256 };

	///////////////////////////////////////////////////////
	// class super_block_t
	///////////////////////////////////////////////////////

	template<size_t BIN_SIZE>
	super_block_t<BIN_SIZE>::super_block_t()
	{
		_init();
	}

	template<size_t BIN_SIZE>
	void * super_block_t<BIN_SIZE>::new_block(size_t count)
	{
		if (!blocks_available())
		{
			throw std::exception("out of blocks");
		}

		if (count > BIN_SIZE)
		{
			throw std::invalid_argument("incorrect block size");
		}

		void * result = header.avail;
		header.avail = header.avail->next;
		size--;
		return result;
	}

	template<size_t BIN_SIZE>
	void super_block_t<BIN_SIZE>::free_block(void * block)
	{
		if (block == nullptr)
		{
			return;
		}

		if (!check_block_ownership(block))
		{
			throw std::invalid_argument("foreign block!");
		}

		_free_block_no_check(block);
		_delete_queued_blocks();
	}

	template<size_t BIN_SIZE>
	void super_block_t<BIN_SIZE>::_init()
	{
		for (size_t index = 1; index < MAX_BINS; ++index)
		{
			blocks[index - 1].next = &blocks[index];
		}
		blocks[MAX_BINS - 1].next = nullptr;
		header.avail = blocks;
	}

	template<size_t BIN_SIZE>
	void super_block_t<BIN_SIZE>::_free_block_no_check(void * block)
	{
		bin_sized_t * bin_block = (bin_sized_t *)block;
		bin_block->next = header.avail;
		header.avail = bin_block;
		header.blocks_avail++;
	}

	template<size_t BIN_SIZE>
	void super_block_t<BIN_SIZE>::_delete_queued_blocks()
	{
		while (!header.blocks_to_delete.empty())
		{
			void * block = blocks_to_delete.front();
			blocks_to_delete.pop();
			_free_block_no_check(block);
		}
	}

	///////////////////////////////////////////////////////
	// class main_block_pool
	///////////////////////////////////////////////////////

	void main_block_pool::init_local_pool(super_blocks_bin local_blocks[])
	{
		for (size_t index = 0; index < BIN_SIZES_COUNT; ++index)
		{
			local_blocks[index].add_block(get_super_block(bin_sizes[index]));
		}
	}

	_sb_ptr main_block_pool::get_super_block(size_t bin_size)
	{
		size_t index = _get_index_for_size(bin_size);
		if (index == -1)
		{
			throw std::invalid_argument("unknown bin size");
		}

		auto block = blocks[index].get_block();
		used_blocks.insert(block);
		return block;
	}

	void main_block_pool::free_super_block(_sb_ptr block)
	{
		auto it = used_blocks.find(block);
		if (it == used_blocks.end())
		{
			throw std::invalid_argument("alien block");
		}

		used_blocks.erase(it);

		size_t index = _get_index_for_size(block->get_block_size());
		blocks[index].add_block(block);
	}

	size_t main_block_pool::_get_index_for_size(size_t bin_size) const
	{
		for (size_t index = 0; index < BIN_SIZES_COUNT; ++index)
		{
			if (bin_sizes[index] == bin_size)
			{
				return index;
			}
		}
		return -1;
	}

	main_block_pool::main_block_pool()
	{
		blocks[0].add_block(new super_block_t<8>());
		blocks[1].add_block(new super_block_t<16>());
		blocks[2].add_block(new super_block_t<32>());
		blocks[3].add_block(new super_block_t<64>());
		blocks[4].add_block(new super_block_t<128>());
		blocks[5].add_block(new super_block_t<256>());
	}

	void * main_block_pool::get_big_block(size_t n_bytes)
	{
		return malloc(n_bytes);
	}

	///////////////////////////////////////////////////////
	// class local_block_pool
	///////////////////////////////////////////////////////

	void * local_block_pool::new_block(size_t n_bytes)
	{
		size_t index = _get_bin_for_size(n_bytes);

		if (index == -1)
		{
			return main_pool.get_big_block(n_bytes);
		}

		return blocks[index].new_mem(n_bytes);
	}

	void local_block_pool::free_block(void * mem)
	{
		size_t index = _get_bin_for_mem(mem);
		if (index == -1)
		{
			main_pool.free_foreign_block(mem);
			return;
		}

		blocks[index].free_mem(mem);
	}

	size_t local_block_pool::_get_bin_for_size(size_t n_bytes) const
	{
		for (size_t index = 0; index < BIN_SIZES_COUNT; ++index)
		{
			if (blocks[index].get_block_size() > n_bytes)
			{
				return index;
			}
		}

		return -1;
	}

	///////////////////////////////////////////////////////
	// class super_blocks_bin
	///////////////////////////////////////////////////////

	void super_blocks_bin::_sort()
	{
		auto cmp = [](const _sb_ptr first, const _sb_ptr second)
		{
			return first->avail_space() < second->avail_space();
		};
		std::sort(blocks.begin(), blocks.end(), cmp);
	}

	void super_blocks_bin::add_block(_sb_ptr block)
	{
		blocks.push_back(block);
		_sort();
	}

	_sb_ptr super_blocks_bin::get_block_space_avail() const
	{
		auto it = blocks.begin();
		while ((*it)->avail_space() == 0)
		{
			if (++it == blocks.end())
			{
				return nullptr;
			}
		}

		return *it;
	}

	size_t super_blocks_bin::get_block_size() const
	{
		return blocks.front()->get_block_size();
	}



	///////////////////////////////////////////////////////
	// class super_block_storage
	///////////////////////////////////////////////////////
	_sb_ptr super_block_storage::get_block()
	{
		if (blocks.size() == 1)
		{
			blocks.push(blocks.front()->create_block());
		}

		auto res = blocks.front();
		blocks.pop();
		return res;
	}

	void super_block_storage::add_block(_sb_ptr block)
	{
		blocks.push(block);
	}
}