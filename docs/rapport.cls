\ProvidesClass{rapport}[2012/06/27]

%\DeclareOption{chapter}{%
%  \PassOptionsToPackage{chapter}{common}%
%}
\DeclareOption{final}{%
  \setlength{\overfullrule}{0pt}%
  \AtBeginDocument{\ShiftPage\let\draft@date=\@empty}%
}
\def\TypeSize{11pt}
\DeclareOption{9pt}{\def\TypeSize{9pt}}
\DeclareOption{10pt}{\def\TypeSize{10pt}}
\DeclareOption{11pt}{\def\TypeSize{11pt}}
\DeclareOption{12pt}{\def\TypeSize{12pt}}

\DeclareOption{twoside}{\ifdim\overfullrule=0pt
  \evensidemargin=1in%
  \PassOptionsToPackage{\CurrentOption}{common}%
\fi}
\DeclareOption*{\PassOptionsToPackage{\CurrentOption}{common}}

\AtBeginDocument{\SetMargins}
\ProcessOptions

\PassOptionsToPackage{\TypeSize}{common}
\RequirePackage{common}
\RequirePackage{math_regler}

\def\sectionTocMark#1#2{\sectionToc{\thesection.}{#2}}
\def\subsectionTocMark#1#2{\subsectionToc{\thesubsection}{#2}}

\@ifundefined{thechapter}{%  Sections only.
  \def\thesubsection{\thesection.\arabic{subsection}}
  \numbered{subsection}
  \def\l@section#1#2{%
    \@dottedtocline{1}{2em}{2em}{\bgroup\bfseries #1}{#2\egroup}}
  \def\l@subsection{\@dottedtocline{2}{4em}{2.5em}}
  \def\l@subsubsection{\@dottedtocline{3}{6em}{2em}}
  \newcommand*\l@figure{\@dottedtocline{1}{2em}{2em}}
  \def\tableofcontents{%
    \begingroup
      \let\sectionTocMark\@gobbletwo
      \section*{\contentsname}
    \endgroup
    \@starttoc{toc}%
    \cleardoublepage
  }
  \newcommand\listoffigures{%
    \begingroup
      \let\chapterTocMark\@gobbletwo
      \section*{\listfigurename}
    \endgroup
    \@starttoc{lof}%
    \cleardoublepage
  }
}{%  Chapters
  \newsection{chapter}%
  \def\chapterSkip{\z@}
  \def\chapterStyle{\clearpage\thispagestyle{nohead}\huge\bfseries}%
  \def\chapterSkipAfter{2\bigskipamount}%
  \def\chapterIdent{\if@num\@hangfrom{\thechapter.\enskip}\fi}%
  \numbered{chapter}%
  \def\chapterTocMark#1#2{%
    \markboth{\if@num\@chapapp\ \thechapter.\quad\fi #1}%
             {\if@num\@chapapp\ \thechapter.\quad\fi #1}%
    \chapterToc{\thechapter.}{#2}}
  \def\sectionTocMark#1#2{%
    \markright{\if@num\thesection\quad\fi#1}%
    \sectionToc{\thesection}{#2}}
    \def\subsectionTocMark#1#2{}
  \AtEndOfClass{\pagestyle{head}}
  \theoremswithin{chapter}
  \def\l@chapter#1#2{\smallskip
    \@dottedtocline{0}{0em}{2em}{\bgroup\bf #1}{#2\egroup}}
  \def\l@section{\@dottedtocline{1}{2em}{2em}}
  \def\l@subsection{\@dottedtocline{2}{4em}{2.5em}}
  \def\l@subsubsection{\@dottedtocline{3}{6em}{2em}}
  \newcommand*\l@figure{\@dottedtocline{1}{2em}{2em}}
  \def\@ppendix{\par
     \setcounter{chapter}{0}\def\@chapapp{\appendixname}%
     \def\thechapter{\Alph{chapter}}}
  \def\tableofcontents{%
    \begingroup
      \let\chapterTocMark\@gobbletwo
      \chapter*{\contentsname}
    \endgroup
    \@starttoc{toc}%
    \cleardoublepageempty
    }
  \newcommand\listoffigures{%
    \begingroup
      \let\chapterTocMark\@gobbletwo
      \chapter*{\listfigurename}
    \endgroup
    \@starttoc{lof}%
    \cleardoublepage
  }
}

\def\noTOC#1{
  \let\savedchapterTocMark\chapterTocMark
  \def\chapterTocMark##1##2{%
     \markboth{\if@num\@chapapp\ \thechapter.\quad\fi##1}{}}
  #1
  \let\chapterTocMark\savedchapterTocMark
}

\newcommand\includedocdata[1]{%
  \setcounter{page}{2}
  \thispagestyle{empty}%
  \vspace*{-26mm}%
  \vbox to 0pt{%
    \centerline{\kern 14mm\includegraphics{#1}}
    \vss}
  \cleardoublepage}
 

\newcommand{\@pnumwidth}{1.55em}
\newcommand{\@tocrmarg} {2.55em}
\newcommand{\@dotsep}{4.5}
\setcounter{tocdepth}{5}

\def\ps@head{\let\@mkboth\markboth
  \def\@oddfoot{\upshape\draft@date\hfil\thepage}%
  \def\@evenfoot{\upshape\thepage\hfil\draft@date}%
  \def\@evenhead{\itshape \leftmark \hfil}%
  \def\@oddhead{\hbox{}\itshape \hfil\rightmark}% Right heading.
}  
\newcommand\cleardoublepageempty{%
  \clearpage\if@twoside\ifodd\c@page\else\hbox{}\thispagestyle{empty}
  \newpage\fi\fi}

    
\ps@nohead
%\topmargin=22mm\headheight=\z@\headsep=\z@
\textheight=240mm
\textwidth=130mm
\def\@chapapp{\chaptername}

