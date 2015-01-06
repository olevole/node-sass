#include <vector>
#include "sass_async_worker.h"

SassAsyncWorker::SassAsyncWorker(NanCallback *callback, sass_context_wrapper* ctx_w, WorkerType type)
  : NanAsyncWorker(callback), context_wrapper(ctx_w), worker_type(type) {}

SassAsyncWorker::SassAsyncWorker(NanCallback *callback, sass_context_wrapper* ctx_w, std::vector<sass_context_wrapper*>* imports, WorkerType type)
  : NanAsyncWorker(callback), context_wrapper(ctx_w), imports_collection(imports), worker_type(type) {}

SassAsyncWorker::~SassAsyncWorker(){
  if (worker_type != CompileWorker){
    return;
  }

  sass_wrapper_dispose(context_wrapper, 0);

  NanAsyncWorker::~NanAsyncWorker();
}

#pragma region Execution

void SassAsyncWorker::Execute(){
  switch (worker_type)
  {
  case CompileWorker:
    ExecuteForCompiler();  break;
  case FunctionWorker:
    ExecuteForFunction();  break;
  case ImportWorker:
    ExecuteForImporter();
  default:
    break;
  }
}

void SassAsyncWorker::ExecuteForCompiler(){
  if (context_wrapper->dctx) {
    compile_data(context_wrapper->dctx);
  }
  else{
    compile_file(context_wrapper->fctx);
  }
}

void SassAsyncWorker::ExecuteForFunction(){
  throw new std::exception("FeatureNotImplementedException: Custom Importer is not implemented.");
}

void SassAsyncWorker::ExecuteForImporter(){
}

#pragma endregion

#pragma region OK Callbacks

void SassAsyncWorker::HandleOKCallback(){
  switch (worker_type)
  {
  case CompileWorker:
    CompilerOKCallback();  break;
  case FunctionWorker:
    FunctionOKCallback();  break;
  case ImportWorker:
    ImporterOKCallback();
  default:
    break;
  }
}

void SassAsyncWorker::CompilerOKCallback(){
  NanScope();

  TryCatch try_catch;
  struct Sass_Context* ctx;

  if (context_wrapper->dctx) {
    ctx = sass_data_context_get_context(context_wrapper->dctx);
  }
  else {
    ctx = sass_file_context_get_context(context_wrapper->fctx);
  }

  int status = GetResult(context_wrapper, ctx);

  if (status == 0) {
    callback->Call(0, 0);
  }
  else {
    const char* err = sass_context_get_error_json(ctx);
    Local<Value> argv[] = {
      NanNew<String>(err),
      NanNew<Integer>(status)
    };
    context_wrapper->error_callback->Call(2, argv);
  }
  if (try_catch.HasCaught()) {
    node::FatalException(try_catch);
  }
}

void SassAsyncWorker::FunctionOKCallback(){
  throw new std::exception("FeatureNotImplementedException: Custom Importer is not implemented.");
}

void SassAsyncWorker::ImporterOKCallback(){
  NanScope();

  imports_collection->push_back(context_wrapper);

  Handle<Value> argv[] = {
    NanNew<String>(strdup(context_wrapper->file ? context_wrapper->file : 0)),
    NanNew<String>(strdup(context_wrapper->prev ? context_wrapper->prev : 0)),
    NanNew<Number>(imports_collection->size() - 1)
  };

  NanNew<Value>(callback->Call(3, argv));
}

#pragma endregion
