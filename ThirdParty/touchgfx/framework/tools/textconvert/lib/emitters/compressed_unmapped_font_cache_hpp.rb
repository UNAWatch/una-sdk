# Copyright (c) 2018(-2025) STMicroelectronics.
# All rights reserved.
#
# This file is part of the TouchGFX 4.26.0 distribution.
#
# This software is licensed under terms that can be found in the LICENSE file in
# the root directory of this software component.
# If no LICENSE file comes with this software, it is provided AS-IS.
#
###############################################################################/
class CompressedUnmappedFontCacheHpp < Template
  def initialize(text_entries, typographies, language, output_directory, compressed_font_cache_size)
    super(text_entries, typographies, language, output_directory)
    @compressed_font_cache_size = compressed_font_cache_size
    @cache = {}
  end
  def input_path
    File.join(root_dir,'Templates','CompressedUnmappedFontCache.hpp.temp')
  end
  def output_path
    '/include/fonts/CompressedUnmappedFontCache.hpp'
  end
  def cache_file
    File.join(@output_directory, 'cache/CompressedUnmappedFontCacheHpp.cache')
  end
  def output_filename
    File.join(@output_directory, output_path)
  end
  def run
    @cache["cache_size"] = @compressed_font_cache_size
    new_cache_file = false
    if not File::exists?(cache_file)
      new_cache_file = true
    else
      #cache file exists, compare data with cache file
      old_cache = JSON.parse(File.read(cache_file))
      new_cache_file = (old_cache != @cache)
    end
    if new_cache_file
      #write new cache file
      FileIO.write_file_silent(cache_file, @cache.to_json)
    end
    if !File::exists?(output_filename) || new_cache_file
      #generate CompressedUnmappedFontCache.hpp
      super
    end
  end

  def get_cache_size
    @compressed_font_cache_size
  end
end
