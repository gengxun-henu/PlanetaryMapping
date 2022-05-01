//  [5/15/2014 gengxun]�����ļ�������ʹ�ã���Ϊʹ��QImgMatchTH�еĹ���

#ifndef QIMGMATCH_ESTIMATE_H
#define QIMGMATCH_ESTIMATE_H

/*
#include "qimgmatch_global.h"

#include "gdal_priv.h"

#include <string>
#include <vector>
using namespace std;

// һ������£���������ص������һ��������ͬ���㣬�����������һ�㣬�ɰٷְٸ�������һ��
enum EMATCH_RESULT
{
	EMATCH_RESULT_NONE = 0,
	EMATCH_RESULT_GOOD = 1,		// �ȽϿ��ţ�Ԥ�������ʹ��NCCƥ��ó���������1������
	EMATCH_RESULT_NORM = 2,		// ����һ�㣬����֪��ֱ��Ԥ��ó�����������֪�������һ��Ϊ2~3����
	EMATCH_RESULT_BAD  = 3,		// Ԥ�������ã��糬���߽磬�����������ؾ���̫���糬��1000������
	EMATCH_RESULT_FAIL = 4,     // Ԥ��ʧ�ܣ������룬�������������
};

namespace ImgMatch
{
	struct MyPt2
	{
		int id;
		double x;
		double y;
	};


	struct ImgData
	{
		int width;
		int height;
		GByte* pImgData;
	};


	// Ĭ��������ƥ�䴰�ڴ�С��Ϊ11*11����������ʱ���������ڿ���СһЩ������7*7������������Ȼʹ��11*11
	struct NCC
	{
		int nMatchWindowWidth;		
		int nMatchWindowHeight;
		int nSearchWindowWdith;
		int nSearchWindowHeight;
		double dfThreashold;      // Ĭ��Ϊ0.75
	};

	struct SplitPara
	{
		int nWidth;
		int nHeight;
		int nOverlapX;
		int nOverlapY;
	};
}


// ref--reference
// norm--normalization
// ori--original
// src--source

//////////////////////////////////////////////////////////////////////////
// �û����ô�����е�λ���ƣ��������£�
// 1. ������ƥ��ͼ��
// 2. ����ƥ������Ҫ�����ļ����ڲ�������Ч�Լ��
// 3. ����ƥ�亯��������ԭʼͼ�񡢷ֿ��ͼ��δ��һ�������ֿ���һ��ͼ��Ԥ����ο�ͼ���ϵĵ�����

class QIMGMATCH_EXPORT QImgMatchEstimate
{
public:
	QImgMatchEstimate(void);
	~QImgMatchEstimate(void);

public:
	//////////////////////////////////////////////////////////////////////////
	// �û��ӿ�
	bool setMatchImg(const string& strRefImgNorm, const string& strMatchImgNorm);
	bool setMatchParameter(const string& strNormParaFile, const string& strRefImgNormKey, const string& strMatchImgNormKey);
	
	// ������һ��ƥ��ͼ����һ�㣬���Ƴ���һ���ο�ͼ����һ��
	EMATCH_RESULT normMatchImgToNormRefImg(double x, double y, double& mx, double &my);
	// �����ֿ����ص�����ƥ��ͼ����һ�㣬���Ƴ���һ���ο�ͼ����һ�㣨��һ���ο�ͼ����ֿ��ص�����ͼ����ͬ��
	EMATCH_RESULT overlapMatchImgToOverlapRefImg(double x1, double y1, double&x2, double& y2);
	// ����ԭʼƥ��ͼ����һ�㣬���Ƴ���һ���ο�ͼ����һ�㣨��ӵ�������������
	EMATCH_RESULT oriMatchImgToNormRefImg(double x1, double y1, double& x2, double& y2);

	// ������֪��Ķ��٣����ò�ͬNCC�����ʱNCC��СһЩ
	void setNCC(ImgMatch::NCC& ncc) {_ncc = ncc;}
	void setDefaultNCC();
	void setDefaultSmallNCC();
	
	// �ⲿ�������ã��жϴ򿪵�ͼ���Ƿ����ʹ�õ�ǰ������ƥ���ϵ����Ԥ�⣬�������GFB�����Ƿ���������
	bool IsCurrentMatchImg(const string& strFile);
	bool canBeMatched(const string& strFile, vector<QImgMatchEstimate*>& vecImgMatcher ); // �ļ������Ƿ����ƥ��ͼ����ο�ͼ��
	bool IsOverlapImg(const string& strFile);	// �ļ��Ƿ���ƥ��ͼ���Overlapͼ��
	bool IsNormalizedReferenceImage(const string& strFile); // �ļ��Ƿ��ǲο�ͼ��Ĺ�һ��ͼ��

	// ��ԭʼͼ������ת����Overlap�ֿ�ͼ�����꣨���ƥ��ͼ����GFB��
	bool FromOriToOverlap(double xOri, double yOri, double& xOverlap, double& yOverlap);

	// �ӷֿ�ͼ������ת����ԭʼ��ͼ�����꣨���ƥ��ͼ����GFB��
	bool FromOvarlapToOri(double xOverlap, double yOverlap, double& xOri, double& yOri);

	// ���ҷֿ�ͼ���Ӧ���С���
	bool FindSplitRowCol( const string& file, int& row, int& col );

protected:
	//////////////////////////////////////////////////////////////////////////
	// ����Ϊ�ڲ�����
	bool readKeyFile(const string& keyFile, vector<ImgMatch::MyPt2>& vecKeypoint, int& nWidth, int& nHeight);
	bool readNormParaFile(const string& normParafile);

	string getShortFilename(const string& strFilepath, bool bHasExt = false);

	bool readImgData(const string& imgFile, ImgMatch::ImgData& imgData);
	void clearImgData();

	bool readSplitInfo(const string& splitFile, ImgMatch::SplitPara& splitPara);

	//////////////////////////////////////////////////////////////////////////
	// ��λ���ƣ����ڽ������
	bool estimatePosition(const ImgMatch::MyPt2& keypoint, const vector<ImgMatch::MyPt2>& vecKeypoint1, const vector<ImgMatch::MyPt2>& vecKeypoint2, ImgMatch::MyPt2& pointEstimated);
	bool searchNearestPoints(const ImgMatch::MyPt2 & point, const vector<ImgMatch::MyPt2>& vecKeyPointTotal, double searchRadiusInPixels, int nMaxIterTimes, int nMinPointsNeeded, vector<ImgMatch::MyPt2>& nearestPoints, vector<int>& nearestPointsID);
	
	bool searchNearestPointsByKDTree(const ImgMatch::MyPt2 & point );
	//////////////////////////////////////////////////////////////////////////
	// �����������
	int  Gauss(double *a,double *b,int n);
	void transpose(double *a,double *b, int m, int n);
	void Multiply(double *a,double *b,int m,int n,int k,double *c);	

	//////////////////////////////////////////////////////////////////////////
	// NCCƥ��
	double NormalizedCrossCorrelation(void* pData1, void* pData2, GDALDataType eDataType,
		int nWidth1, int nHeight1, int nWidth2, int nHeigh2,
		int x1, int y1, int x2, int y2,
		int nMatchWindowWidth, int nMatchWindowHeight);
	bool NCCMatch(void* pData1, void* pData2, GDALDataType eDataType,	// ͼ������
		int nWidth1, int nHeight1, int nWidth2, int nHeight2,	// ͼ���ȡ��߶�
		int x1, int y1, int pre_x2, int pre_y2,					// ��Ӱ��x1,y1����Ԥ����Ӱ���λx2,y2
		int nMatchWindowWidth, int nMatchWindowHeight,			// ƥ�䴰��
		int nSearchWindowWidth, int nSearchWindowHeight,		// ��������
		double dfThresholdNCC,									// ���ϵ����ֵ
		int& match_x2, int& match_y2							// ƥ����ĵ�λ����Ϊ-1��-1��ʾδƥ��ɹ�
		);
	double NormalizedCrossCorrelationGByte(const vector<GByte>& vecImgData1, const vector<GByte>& vecImgData2, int nWindowWidth, int nWindowHeight);
	double NormalizedCrossCorrelationGUInt16(const vector<GUInt16>& vecImgData1, const vector<GUInt16>& vecImgData2, int nWindowWidth, int nWindowHeight);

private:
	string _strNormParaFile;			// ��һ�������ļ�
	string _strOriRefImg;				// ԭʼ�ο�ͼ���ļ�·��
	string _strOriMatchImg;				// ԭʼƥ��ͼ���ļ�·��
	string _strRefImgNormKey;			// �ο�ͼ���һ�����ƥ����ļ�
	string _strMatchImgNormKey;			// ƥ��ͼ���һ�����ƥ����ļ�
	string _strRefImgNorm;				// �ο�ͼ���һ������ļ�·����һ���ֿ飩
	string _strMatchImgNorm;			// ƥ��ͼ���һ������ļ�·����һ���ֿ飩

	ImgMatch::ImgData _refImgData;				// ��һ���ο�ͼ������
	ImgMatch::ImgData _matchImgData;			// ��һ��ƥ��ͼ������

	ImgMatch::NCC _ncc;

	//////////////////////////////////////////////////////////////////////////
	// �ɹ�һ�������ļ��л�ȡ
	double _scale;								// ƥ��ͼ���������ο�ͼ��
	// ԭʼƥ��ͼ���ص�����
	int _nLeftMatch;						
	int _nTopMatch;
	int _nRightMatch;
	int _nBottomMatch;
	// ԭʼ�ο�ͼ���ص�����
	int _nLeftRef;
	int _nTopRef;
	int _nRightRef;
	int _nBottomRef;

	//////////////////////////////////////////////////////////////////////////
	// �ɹ�һ��ͼ��ƥ����ļ��л�ȡ
	vector<ImgMatch::MyPt2> _vecPtRefNorm;		// �ο�ͼ���һ�����ƥ���
	vector<ImgMatch::MyPt2> _vecPtMatchNorm;		// ƥ��ͼ���һ�����ƥ���

	int _nNormImgWidthMatch;		// ��һ����ƥ��ͼ��һ���ֿ�Ŀ�ȣ���key�ļ���Ӧ
	int _nNormImgHeightMatch;		// ��һ����ƥ��ͼ��һ���ֿ�ĸ߶ȣ���key�ļ���Ӧ
	int _nNormImgWidthRef;			// ��һ����ο�ͼ��һ���ֿ�Ŀ�ȣ���key�ļ���Ӧ		
	int _nNormImgHeightRef;			// ��һ����ο�ͼ��һ���ֿ�ĸ߶ȣ���key�ļ���Ӧ

	// �ָ�ͼ����Ϣ
	int _nCurSplitRow;
	int _nCurSplitCol;

	ImgMatch::SplitPara _MatchImgOverlapSplitInfo; // ԭʼͼ��Overlap�ص�����δ�仯�ֱ��ʣ��ָ���Ϣ
	//ImgMatch::SplitPara _MatchImgNormSplitInfo;    // ԭʼͼ���һ����ķָ���Ϣ
	ImgMatch::SplitPara _RefImgNormSplitInfo;	   // �ο�ͼ���һ����ķָ���Ϣ
};

*/

#endif // QIMGMATCH_ESTIMATE_H
