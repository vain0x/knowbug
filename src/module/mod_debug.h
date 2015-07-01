// デバッグ向けモジュール (for clhsp)

#ifndef IG_MODULE_TO_DEBUG_FOR_CLHSP_H
#define IG_MODULE_TO_DEBUG_FOR_CLHSP_H

#ifdef HSPDEBUG

# define Msgboxf CMsgboxf()
	class CMsgboxf {
	public:
		int operator()(const char* pFormat, ...);
	};
	
# define runerr /* throw */ CThrowWrapper( __FILE__, __LINE__ ) % /* exception */
# define cmperr runerr
# define no_err CThrowWrapper::CNoError() %
	class CThrowWrapper
	{
		const char* mpFile;
		int miLine;
		bool mbMsgbox;
		
	public:
		class CNoError
		{
			friend CThrowWrapper;
			const CThrowWrapper* mpWrapper;
			
		public:
			CNoError()
				: mpWrapper( 0 )
			{ }
			
			template<class T>
			const T& operator %( const T& exception ) const
			{
				return (*mpWrapper) % exception;
			}
		};
		
	public:
		CThrowWrapper( const char* pFile, int iLine )
			: mpFile( pFile )
			, miLine( iLine )
			, mbMsgbox( true )
		{ }
		
		// 優先度が高いため、% を用いる ( おそらく << とかの方が適切 )
		template<class T>
		const T& operator %( const T& exception ) const
		{
			if ( mbMsgbox ) Msgboxf( "clhspエラー\n\n[%s]\nline: %d\n", mpFile, miLine );
			return exception;
		}
		
		template<>
		const CNoError& operator %( const CNoError& obj ) const
		{
			const_cast<CNoError&>(obj).mpWrapper = this;
			const_cast<CThrowWrapper*>(this)->mbMsgbox = false; 
			return obj;
		}
	};
	
#else
# define runerr
# define cmperr
# define no_err
# define Msgboxf(format, ...) ((void)0)
#endif

#endif
