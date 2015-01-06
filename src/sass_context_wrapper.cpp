#include "sass_context_wrapper.h"

extern "C" {
  using namespace std;

  void compile_it(uv_work_t* req) {
    sass_context_wrapper* ctx_w = (sass_context_wrapper*)req->data;

    if (ctx_w->dctx) {
      compile_data(ctx_w->dctx);
    }
    else if (ctx_w->fctx) {
      compile_file(ctx_w->fctx);
    }
  }

  void compile_data(struct Sass_Data_Context* dctx) {
    sass_compile_data_context(dctx);
  }

  void compile_file(struct Sass_File_Context* fctx) {
    sass_compile_file_context(fctx);
  }

  void GetStats(sass_context_wrapper* ctx_w, Sass_Context* ctx) {
    NanScope();

    char** included_files = sass_context_get_included_files(ctx);
    Handle<Array> arr = NanNew<Array>();

    if (included_files) {
      for (int i = 0; included_files[i] != nullptr; ++i) {
        arr->Set(i, NanNew<String>(included_files[i]));
      }
    }

    NanNew(ctx_w->result)->Get(NanNew("stats"))->ToObject()->Set(NanNew("includedFiles"), arr);
  }

  void GetSourceMap(sass_context_wrapper* ctx_w, Sass_Context* ctx) {
    NanScope();

    Handle<Value> source_map;

    if (sass_context_get_error_status(ctx)) {
      return;
    }

    if (sass_context_get_source_map_string(ctx)) {
      source_map = NanNew<String>(sass_context_get_source_map_string(ctx));
    }
    else {
      source_map = NanNew<String>("{}");
    }

    NanNew(ctx_w->result)->Set(NanNew("sourceMap"), source_map);
  }

  int GetResult(sass_context_wrapper* ctx_w, Sass_Context* ctx) {
    NanScope();

    int status = sass_context_get_error_status(ctx);

    if (status == 0) {
      NanNew(ctx_w->result)->Set(NanNew("css"), NanNew<String>(sass_context_get_output_string(ctx)));

      GetStats(ctx_w, ctx);
      GetSourceMap(ctx_w, ctx);
    }

    return status;
  }

  sass_context_wrapper* sass_make_context_wrapper() {
    sass_context_wrapper* ctx_w = (sass_context_wrapper*)calloc(1, sizeof(sass_context_wrapper));
    uv_mutex_init(&ctx_w->importer_mutex);
    uv_cond_init(&ctx_w->importer_condition_variable);

    return ctx_w;
  }

  void sass_wrapper_dispose(struct sass_context_wrapper* ctx_w, char* string = 0) {
    if (ctx_w->dctx) {
      sass_delete_data_context(ctx_w->dctx);
    }
    else if (ctx_w->fctx) {
      sass_delete_file_context(ctx_w->fctx);
    }

   delete ctx_w->error_callback;
    delete ctx_w->importer_callback;
    delete ctx_w->file;
    delete ctx_w->prev;

    uv_mutex_destroy(&ctx_w->importer_mutex);
    uv_cond_destroy(&ctx_w->importer_condition_variable);

    NanDisposePersistent(ctx_w->result);

    if(string) {
      free(string);
    }
  }

  void sass_free_context_wrapper(sass_context_wrapper* ctx_w) {
    sass_wrapper_dispose(ctx_w);

    free(ctx_w);
  }
}
