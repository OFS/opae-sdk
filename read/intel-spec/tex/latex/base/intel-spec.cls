% Adapted by Steve King from the tufty-handout.cls file.
%
% Do not distribute this file outside of Intel.
%
% No license is granted for graphics, fonts, or any other external files
% that may be required to use this Latex class.
% All such external files remain the exclusive property of Intel Corporation.
%
% The original file has the following copyright and license:
%
% Copyright 2007-2008 Bil Kleb, Bill Wood and Kevin Godby
%
% Licensed under the Apache License, Version 2.0 (the "License");
% you may not use this file except in compliance with the License.
% You may obtain a copy of the License at
%
%    http://www.apache.org/licenses/LICENSE-2.0
%
% Unless required by applicable law or agreed to in writing, software
% distributed under the License is distributed on an "AS IS" BASIS,
% WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
% See the License for the specific language governing permissions and
% limitations under the License.

% revision history
% 2008/12/10 - 0.1     Initial release
% 2012/7/10  - 0.2     Updated Intel Blue RGB values
% 2013/4/22 - 0.3      Wide floats are now flush left with a caption that does not
%                      execeed the normal body text width.
%                      Changed the header to remove the Intel Logo due to inadequate
%                         space around the logo to meet branding guidelines.
%                      Moved the title page logo up and right to comply with branding
%                         guidelines.
%                      Removed Intel Blue from Intel font.  Intel Blue must now be
%                         explicitly specified if needed with Intel font.
%                      Make all header text and header rule Intel Blue
%                      Added confidential or secret to chapter title pages
%                      Added chapter title to header
%                      "Intel Secret" now correctly appears in red in the header
% July 2, 2014 - 0.5   Update to Intel Clear font from Yu-yuan Chen.


\NeedsTeXFormat{LaTeX2e}[1994/06/01]
\ProvidesClass{intel-spec}[2012/7/10 v0.5 Intel Spec class]

\DeclareOption*{\PassOptionsToClass{\CurrentOption}{book}}
\ProcessOptions
\LoadClass[twoside,openany]{book}

% Other packages sometimes fail silently without calc.  Don't remove it.
\RequirePackage{calc}

\RequirePackage{fixltx2e}
\RequirePackage[usenames,dvipsnames]{xcolor}
\RequirePackage{booktabs}  % booktabs provides toprule, midrule, etc
\RequirePackage[hiresbb]{graphicx} % pulled in by xltxtra
%\DeclareGraphicsExtensions{.pdf,.jpeg,.png}
\RequirePackage[strict]{chngpage}
\RequirePackage{ragged2e}
\RequirePackage{paralist}
\RequirePackage{titlesec}	% Formatting for section titles
\RequirePackage{needspace}
\RequirePackage{xspace}
\RequirePackage{natbib}
\RequirePackage{bibentry}	% allows bibitems to be typeset outside thebibliography environment
% the best monospaced font for \texttt is inconsolata - much nicer than Courier family
% The Latex font name for inconsolata is "fi4" for some reason.  Sometimes this fi4 name pops up
% in error messages.
\RequirePackage{ifxetex}

\RequirePackage{xifthen}
%%
% The placeins package provides the \FloatBarrier command.  This forces
% LaTeX to place all of the floats before proceeding.  We'll use this to
% keep the float (figure and table) numbers in sequence.
\RequirePackage{placeins}


% ***************************************************************************

% Define the Intel Blue color.
% Intel Branding defines these values.  Please do not change them.
% http://brand.intel.com/Templates/FullMainAreaTemplate.aspx?PPL=CBIDMBGLPRIMARYCOLOR
%
% NOTE: YOU MUST ALSO SYNC THE INTEL FONT COLOR IN HEX BELOW!!
%
\xglobal\definecolor{IntelBlue}{RGB}{0,113,197}
% CMYK supposed gives better results for printing, but Latex does not
% understand CMYK in 0-255 format.
%\xglobal\definecolor{IntelBlue}{CMYK}{100,40,0,0}

% ***************************************************************************

% ***************************************************************************
% Define font related bits.

% We relay on xetex for font sanity.
\ifxetex
	\message{**********************************************************^^J}
	\message{intel-spec: XeTeX detected.  Enabling Intel fonts.^^J}
	\message{**********************************************************^^J}
	% load xparse manually before fontspec with the log-declarations=false
	% to suppress useless xparse warnings.
	\RequirePackage[log-declarations=false]{xparse}
	\RequirePackage{fontspec}
% use lining numbers (modern style) rather than old style numbers in libertine
\RequirePackage{libertine} % a nice font with proper ligature handling for correct PDF search
\RequirePackage{droidmono}  % monospace font family, was inconsolata before the inconsolata snafu on CTAN.

% cut and paste from PDF does not work with all linotype ligatures,
% so we have to turn them off
\setmainfont[Mapping=tex-text, Ligatures=NoCommon]{Linux Libertine O}
\setsansfont[Mapping=tex-text, Ligatures=NoCommon]{Linux Biolinum O}
%\setmainfont[Mapping=tex-text]{Droid Serif}
%\setsansfont[Mapping=tex-text]{Droid Sans}
	
	\newfontfamily\intelfont[
		%Color=0071C5,		% Intel Blue RGB converted to hex
		Mapping=tex-text,
		BoldFont=* Bold,
		BoldItalicFont=* Bold Italic,
		ItalicFont=* Italic]
		{Intel Clear}
	
%	\setmainfont{Cambria}  % bad ligature glyph names break PDF search!
%	\setsansfont{Calibri}

	% \setmonofont{Inconsolata} - no longer works in miktex
	\setmonofont[Scale=0.85]{DroidSansMono}
\else
	\message{*****************************************************************^^J}
	\message{Xetex not detected.  Reverting to Latex, but fonts will be ugly. ^^J}
	\message{*****************************************************************^^J}
  % we don't have Intel font unless we run xetex
	\newcommand{\intelfont}{}
\fi
% ***************************************************************************

% Define the total working area in our page
\newlength{\widewidth}
\addtolength{\widewidth}{6.75in}

%%
% Set page layout geometry
% The asymmetric option keeps the margin notes always on the same side of the page which is the way Tufte does it.
\RequirePackage[
	letterpaper,
	asymmetric,
	includemp,
	headheight=0.5in,
	inner=0.5in,
	bindingoffset=1.0in,
	width=\widewidth,
	marginparsep=0.25in,
	marginparwidth=2in,
	bottom=0.5in,
	top=0.1in,
	nofoot,
	% showframe is useful for debug
	%showframe,
	includehead]
	{geometry}

% Suppress Headers and page numbers on empty pages
\RequirePackage{emptypage}

%%
% Separation marginpars by a line's worth of space.

\setlength\marginparpush{\baselineskip}


% Steve - add a bit more space between rows in tables
%\renewcommand{\arraystretch}{1.2}

%%
% Set font and fontsize for margin items
% \newcommand{\@intelspec@marginfont}{\normalfont\footnotesize\sffamily}
\newcommand{\@intelspec@marginfont}{\normalfont\footnotesize}

%%
% Set the amount of raggedness for ragged right text.
% We use Ragged right for justification of certain text,
% such as margin notes.
% this means the preferred raggedness is 0pt, but allow
% up to 2em of tolerance inside the margin.
\setlength{\RaggedRightRightskip}{0pt minus 2em}

% Steve - Allow section numbering to depth 4
% More than this is getting carried away, even for a technical spec.
\setcounter{secnumdepth}{4}
% Set the depth of references in the table of contents to the same.
\setcounter{tocdepth}{4}

%\RequirePackage{marginnote}



% Compute length used for full-width displays

\newlength{\@intelspec@overhang}
\setlength{\@intelspec@overhang}{\marginparwidth}
\addtolength{\@intelspec@overhang}{\marginparsep}


% Add rightmargin to compactenum
% pertains to paralist package
\def\@compactenum@
	{%
  	\expandafter\list\csname label\@enumctr\endcsname
  		{%
    		\usecounter{\@enumctr}%
    		\rightmargin=2em% added this
    		\parsep\plparsep
    		\itemsep\plitemsep
    		\topsep\pltopsep
    		\partopsep\plpartopsep
    		\def\makelabel##1{\hss\llap{##1}}
    		}
    	}


%%
% Transform existing \footnotes into \sidenotes
% Sidenote: ``Where God meant footnotes to go.'' ---Tufte

\RequirePackage[side,multiple,stable]{footmisc}
\providecommand*{\footnotelayout}{\@intelspec@marginfont\justifying}
\renewcommand{\footnotelayout}{\@intelspec@marginfont\justifying}

% Set the size of the footnote marks.
\long\def\@makefntext#1{\@textsuperscript{\@intelspec@marginfont\scriptsize\@thefnmark}\,\footnotelayout#1}
% Set the in-text footnote mark in the same typeface as the body text itself.
\def\@makefnmark{\hbox{\@textsuperscript{\normalfont\scriptsize\@thefnmark}}}

\let\sidenote\footnote

%%
% Sidenote without the footnote mark
\providecommand\marginnote[1]%
  {\marginpar{\@intelspec@marginfont\justifying #1}}

%%
% Citations should go in the margin as well
%\nobibliography*		% pre-loads the bibliography keys
\renewcommand{\cite}[1]{\sidenote{\bibentry{#1}.}}
\providecommand{\doi}[1]{\textsc{doi:} #1} % pre-defining this so it may be used before the \bibliography command it issued

% TODO: Rewrite \cite so that you can specify multiple bib keys
%       at once.  For example, \cite{Author01,Author02}
% TODO: Combine sequences of citations so that 2,3,4,6 becomes 2-4,6
%       but be careful of hyperref interaction


%%
% Format the captions in a style similar to the sidenotes
%\RequirePackage[font={sf,small},textfont={sf,small},labelfont={sf,sc,bf},labelsep=endash]{caption}
%\RequirePackage[font={small},textfont={small},labelfont={bf},labelsep=endash]{caption}
\RequirePackage[font={footnotesize},textfont={footnotesize},labelfont={bf},labelsep=endash]{caption}


%%
% Sets up the page header
% RE = right side of even pages
% LE = left side of even pages
% RO = right side of odd pages
% LO = left side of odd pages
\RequirePackage{fancyhdr}

% Set the default page style to 'fancy'
\pagestyle{fancy}

% Margin note area is on the right side of all pages as per Tufte.
\fancyhfoffset[RE,RO]{\@intelspec@overhang}
\fancyhfoffset[LE,LO]{0in}

% The 'fancy' page style is the default style for most pages
% but not pages that start a new chapter
\fancyhf{} % clear header and footer fields
\fancyhead[RO,LE]{\textcolor{IntelBlue}{\intelfont\thepage}}
\fancyhead[LO,RE]{\textcolor{IntelBlue}{\intelfont\leftmark}}

\renewcommand{\headrulewidth}{0.5pt}
\renewcommand{\footrulewidth}{0pt}

% The 'empty' page style suppresses all headers and footers.
% It's used on title pages and `intentionally blank' pages.
\fancypagestyle{empty}
	{
	\fancyhf{} % clear header and footer fields
	\renewcommand{\headrulewidth}{0pt} % no header rule
	}

\newif\ifConfidential
\Confidentialfalse

\newif\ifSecret
\Secretfalse

% The 'plain' page style is used on the first page of a chapter
% According to Intel brandhing guidelines, 0.75 inches is the minimum
% recommended size for the Intel logo.
\fancypagestyle{plain}
	{
	\fancyhf{} % clear header and footer fields
	\fancyhead[RO,LE]{\textcolor{IntelBlue}{\intelfont\thepage}}
	\ifConfidential
		\fancyhead[C]{\color{IntelBlue}\textbf{\large \intelfont{Intel Confidential}}}
	\fi
	\ifSecret
		\fancyhead[C]{\textcolor{red}{\textbf{\large \intelfont{Intel Top Secret}}}}
	\fi
	\renewcommand{\headrulewidth}{0.5pt}
	\renewcommand{\footrulewidth}{0pt}
	}


\newcommand{\secret}{\fancyhead[C]{\textcolor{red}{\textbf{\large \intelfont{Intel Top Secret}}}}\Secrettrue}
\newcommand{\confidential}{\fancyhead[C]{\color{IntelBlue}\textbf{\large \intelfont{Intel Confidential}}}\Confidentialtrue}

% make the header separator line IntelBlue
\renewcommand{\headrule}{{\color{IntelBlue}%
  \hrule width\headwidth height\headrulewidth \vskip-\headrulewidth}}

% changing chapter marks must come after pagestyle which sets up defaults!
\renewcommand{\chaptermark}[1]{%
\markboth{#1}{}}


\renewcommand\maketitle
	{
	\begin{titlepage}%
	\let\footnotesize\small
	\let\footnoterule\relax
	\@maketitle
	\end{titlepage}%
	% Reset page numbers and footnotes to one for the first
	% page of the body of the document
	\cleardoublepage
	\setcounter{footnote}{0}
	\global\let\maketitle\relax
	\global\let\@author\@empty
	\global\let\@date\@empty
	\global\let\@title\@empty
	\global\let\title\relax
	\global\let\author\relax
	\global\let\date\relax
	\global\let\and\relax
	}


\newcommand{\@revision}{0.0}

\newcommand{\revision}[1]
	{
	\renewcommand{\@revision}{#1}
	}
	
\def\@maketitle
	{
	\hspace*{3.5in}\includegraphics[height=1.5in]{intel_logo}
	\vskip 1in
  	\noindent\textcolor{IntelBlue}{\intelfont\textbf{\huge\@title}
  	\vskip0.5\baselineskip
  	\noindent\color{IntelBlue}\rule{5in}{1pt}
  	\vskip0.5\baselineskip
	%  \noindent\normalsize{\allcaps{\@author}}\\
  	\vskip0.3\baselineskip
  	\noindent{\LARGE Revision \@revision}
  	\vskip0.3\baselineskip
  	\noindent{\LARGE\@date}
	\thispagestyle{empty}
	}}

% *********************************************************************
% Set up section formats.  These are the latex default except for
% the use of Neo Sans Intel font.
\titleformat{\chapter}[display]
{\color{IntelBlue}\intelfont\Huge\bfseries}{\color{IntelBlue}\intelfont\Huge\bfseries\chaptertitlename\ \thechapter}{10pt}{\color{IntelBlue}\Huge}
% The \needspacee argument means that we don't start a new section unless we have
% at least 1 inch of space.  This helps appearance, IMO.
\titleformat{\section}
{\needspace{1in}\color{IntelBlue}\intelfont\Large\bfseries}{\thesection}{1em}{}
\titleformat{\subsection}
{\color{IntelBlue}\intelfont\large\bfseries}{\thesubsection}{1em}{}
\titleformat{\subsubsection}
{\color{IntelBlue}\intelfont\normalsize\bfseries}{\thesubsubsection}{1em}{}
\titleformat{\paragraph}[runin]
{\color{IntelBlue}\intelfont\normalsize\bfseries}{\theparagraph}{1em}{}
\titleformat{\subparagraph}[runin]
{\color{IntelBlue}\intelfont\normalsize\bfseries}{\thesubparagraph}{1em}{}
% *********************************************************************


% *********************************************************************
% Handy bytefield related definitions
\newcommand{\colorbitbox}[3]{%
	\rlap{\bitbox{#2}{\color{#1}\rule{\width}{\height}}}%
	\bitbox{#2}{#3}}
	
% handy gray bitbox
\newcommand{\graybitbox}[2]{%
	\colorbitbox{gray!20}{#1}{#2}}
% *********************************************************************

% *********************************************************************
% Setup handling of floats and tables.
%\RequirePackage[facing=yes]{floatrow}
\RequirePackage{floatrow}

\DeclareFloatSeparators{marginparsep}{\hskip\marginparsep}

% For normal width tables, and figures, place captions in the margin
% the margin note area is always on the right side of a page
% which simplifies this logic
\floatsetup[figure]
	{
	floatwidth=\linewidth,
	capposition=beside,
	capbesideposition={right,center},
	capbesideframe=yes,
	capbesidewidth=\marginparwidth,
	capbesidesep=marginparsep,
	floatrowsep=qquad
	}

\captionsetup[figure]
	{
	font=footnotesize
	}

\floatsetup[table]
	{
	floatwidth=\linewidth,
	capposition=beside,
	capbesideposition={right,center},
	capbesideframe=yes,
	capbesidewidth=\marginparwidth,
	capbesidesep=marginparsep,
	%footnoterule=none,
	footskip=.35\skip\footins,
	floatrowsep=qquad
	}

\captionsetup[table]
	{
	font=footnotesize
	}


% widefloat covers both wide tables and wide figures
% Unless specified here, widetable inherits from
% the floatsetup for table
\floatsetup[widefloat]
{
	margins=hangright,
	capposition=bottom,
	font=small
}

% For readability, don't let the caption of wide floats become wider than normal
% body text width.  Note that if multiple floats exist in a floatrow
% then that table should manually set the margins to 0pt or something smaller
% than 1.125.   For example:
%
%% fix column margins for wide figures with multiple sub-figures.
%\captionsetup{margin={0pt,0pt}}
\captionsetup[widefloat]
{
	margin={1.125in,1.125in}
}

%%
% Margin figure environment

\newsavebox{\@intelspec@marginfigbox}
\newenvironment{marginfigure}[1]
	[-1.2ex]
	{
	\FloatBarrier% process all floats before this point so the figure numbers stay in order.
    	\begin{lrbox}{\@intelspec@marginfigbox}
    	\begin{minipage}{\marginparwidth}
        \@intelspec@marginfont
        \def\@captype{figure}
        \captionsetup{font=footnotesize,hypcap=false}
        \vspace*{#1}
	}
    	{
    	\end{minipage}%
     	\end{lrbox}%
     	\marginpar
     		{
     		\usebox{\@intelspec@marginfigbox}
     		}
     	}

%%
% Tighten up space between displays (e.g., a figure or table) and make symmetric
%\setlength\abovedisplayskip{6pt plus 2pt minus 4pt}
%\setlength\belowdisplayskip{6pt plus 2pt minus 4pt}

%%
% Margin table environment
\newsavebox{\@intelspec@margintablebox}
\newenvironment{margintable}[1]
	[-1.2ex]
	{
	\FloatBarrier%
   	\begin{lrbox}{\@intelspec@margintablebox}
 	\begin{minipage}{\marginparwidth}
 	% narrow the solumn separations for tiny tables
% 	\setlength{\tabcolsep}{2pt}
 	\@intelspec@marginfont
       	\def\@captype{table}
        \captionsetup{font=footnotesize,hypcap=false}
 	\vspace*{#1}
   	}
   	{
   	\end{minipage}%
    	\end{lrbox}%
    	\marginpar
    		{
    		\usebox{\@intelspec@margintablebox}
    		}
    	}


% Steve - macro from 'TH' on tex.stackexchange.com
% Use \smashcaption instead of \caption to avoid extra vertical
% space around figures when  the caption is bigger than the picture.
% see http://tex.stackexchange.com/questions/14272/want-figure-height-to-ignore-side-caption-height
%\newcommand*\smashcaption{
	%\def\FR@makecaption##1##2{%
		%\vskip\abovecaptionskip
			%\vbox to\z@{%
			%\vss
%% \RequirePackage[font={small},textfont={small},labelfont={bf},labelsep=endash]{caption}			
%%			{\captionlabelfont ##1}{\space\textendash\space}{\captiontextfont ##2}%
			%{\textbf{\small ##1}}{\small\space\textendash\space}{\small ##2}%
			%\par
			%\vss
			%}%
		%\vskip\belowcaptionskip
	%}%
	%\caption
%}

\newcommand*\smashcaption{
	\def\FR@makecaption##1##2{%
		\vskip\abovecaptionskip
			% \vbox to \z@ says create a vertical content box,
			% but set the vertical size to zero.  \z@ is '0' in tex.
			% see small clues here: http://infohost.nmt.edu/tcc/help/pubs/texcrib.pdf
			\vbox to\z@{%
			\vss  % infinite glue
% \RequirePackage[font={small},textfont={small},labelfont={bf},labelsep=endash]{caption}			
%			{\captionlabelfont ##1}{\space\textendash\space}{\captiontextfont ##2}%
% OK, this is tricky - the font size in effect at the end of the paragraph
% defines the line space of that paragraph.  To make the caption spaced
% appropriately for the small font size, make sure \small is still in effect
% at the end of this statement.  Mainly, don't wrap any {} around
% the area that \small affects.  The last } must be from \small itself!
			\footnotesize{\textbf{##1}{\space\textendash\space}##2}%
			\par
			\vss
			}%
		\vskip\belowcaptionskip
	}%
	\caption
}

% The Libertine font has an oversize and poorly positioned
% Registered trade-mark.  Fix that.
\newcommand{\registered}{{\raisebox{0.5ex}{\footnotesize\textregistered}}\xspace}


\RequirePackage{ifoddpage}

% This command is a hack to fix the bogus margin handling in floatrow
% for wide floats that use objectset=raggedright.  You must instert
% \widecapfix just above each caption in a wide float.  The resulting caption
% will be correctly left justified.
%
\newcommand{\widecapfix}
{
	\checkoddpage
	\ifoddpage
		\captionsetup{margin={0in,2.25in}}
	\else
		\captionsetup{margin={2.25in,0in}}
	\fi
}

\endinput
