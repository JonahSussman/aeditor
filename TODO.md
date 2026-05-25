# TODO

## Render batching for large clip counts

The single-ffmpeg render approach opens the source video once per clip (`-i`). At 100k+ clips, this hits the OS file descriptor limit (default 1024). For large renders, implement batching:

- Render in chunks of ~500 clips to intermediate files
- Concat the intermediate files in a final pass
- Clean up intermediates after

Relevant code: `Renderer::render()` in `src/aeditor.cpp`, `LoadedTools::render()` in `src/services/service_tools.cpp`.
