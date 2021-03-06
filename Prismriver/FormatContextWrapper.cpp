#include "FormatContextWrapper.h"
#include "AVException.h"

namespace Prismriver
{
	FormatContextWrapper::~FormatContextWrapper()
	{
		if (this->formatContext != nullptr)
		{
			if (this->input_)
			{
				avformat_close_input(&this->formatContext);
			}
			else
			{
				if ((this->ioContextWrapper_ == nullptr) && ((this->formatContext->oformat->flags & AVFMT_NOFILE) == 0))
					avio_closep(&this->formatContext->pb);
				avformat_free_context(this->formatContext);
				this->formatContext = nullptr;
			}
		}

		if (this->file_ != nullptr)
			av_freep(&this->file_);

		if (this->ioContextWrapper_ != nullptr)
		{
			delete this->ioContextWrapper_;
			this->ioContextWrapper_ = nullptr;
		}

		if (this->format_ != nullptr)
			av_freep(&this->format_);
	}

	void FormatContextWrapper::OpenRead()
	{
		if (this->opened_) throw gcnew InvalidOperationException("This FormatContextWrapper had already been opened.");
		this->opened_ = true;
		this->input_ = true;

		this->formatContext = avformat_alloc_context();
		if (this->formatContext == nullptr) throw gcnew OutOfMemoryException();

		if (this->ioContextWrapper_ != nullptr)
		{
			this->ioContextWrapper_->OpenRead(); // Throws on failure
			this->formatContext->pb = this->ioContextWrapper_->ioContext;
		}

		HRESULT hr = avformat_open_input(&this->formatContext, this->file_, nullptr, nullptr);
		if (FAILED(hr)) throw gcnew AVException(hr);
	}

	void FormatContextWrapper::OpenWrite()
	{
		if (this->opened_) throw gcnew InvalidOperationException("This FormatContextWrapper had already been opened.");
		this->opened_ = true;

		if (this->format_ == nullptr && this->file_ == nullptr)
			throw gcnew InvalidOperationException("If wrapping an output stream, explicitly setting an output format is required.");

		AVOutputFormat* pOutputFormat = av_guess_format(this->format_, this->file_, nullptr);

		if (pOutputFormat == nullptr) return throw gcnew Exception("Could not find a suitable output format from the file name or output format.");

		HRESULT hr = avformat_alloc_output_context2(&this->formatContext, pOutputFormat, nullptr, this->file_);
		if (FAILED(hr)) throw gcnew AVException(hr);

		if (this->ioContextWrapper_ != nullptr)
		{
			this->ioContextWrapper_->OpenWrite(); // Throws on failure
			this->formatContext->pb = this->ioContextWrapper_->ioContext;
		}
		else if ((pOutputFormat->flags & AVFMT_NOFILE) == 0)
		{
			hr = avio_open(&this->formatContext->pb, this->file_, AVIO_FLAG_WRITE);
			if (FAILED(hr)) throw gcnew AVException(hr);
		}
	}
}
