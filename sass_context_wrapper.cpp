#include "sass_context_wrapper.h"
#include <nan.h>
#include <cstdlib>

extern "C" {
  using namespace std;

  void free_context(sass_context* ctx) {
    sass_free_context(ctx);
  }

  void free_file_context(sass_file_context* fctx) {
    sass_free_file_context(fctx);
  }

  sass_context_wrapper* sass_new_context_wrapper() {
    return (sass_context_wrapper*) calloc(1, sizeof(sass_context_wrapper));
  }

  void sass_free_context_wrapper(sass_context_wrapper* ctx_w) {
    if (ctx_w->ctx) {
      free_context(ctx_w->ctx);
    } else if (ctx_w->fctx) {
      free_file_context(ctx_w->fctx);
    }

    NanDisposePersistent(ctx_w->stats);
    delete ctx_w->callback;
    delete ctx_w->errorCallback;

    free(ctx_w);
  }
}
