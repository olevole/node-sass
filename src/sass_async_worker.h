#include <node.h>
#include <nan.h>
#include "sass_context_wrapper.h"

using v8::Function;
using v8::Local;
using v8::Null;
using v8::Number;
using v8::Value;

class SassAsyncWorker : public NanAsyncWorker {
public:
  SassAsyncWorker(NanCallback*, sass_context_wrapper*, WorkerType);
  SassAsyncWorker(NanCallback*, sass_context_wrapper*, std::vector<sass_context_wrapper*>*, WorkerType);

  ~SassAsyncWorker();

  // Executed inside the worker-thread.
  // It is not safe to access V8, or V8 data structures
  // here, so everything we need for input and output
  // should go on `this`.
  void Execute();

  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback();

private:
  WorkerType worker_type;
  sass_context_wrapper* context_wrapper;
  std::vector<sass_context_wrapper*>* imports_collection;

  void ExecuteForCompiler();
  void ExecuteForFunction();
  void ExecuteForImporter();
  void CompilerOKCallback();
  void FunctionOKCallback();
  void ImporterOKCallback();
};
