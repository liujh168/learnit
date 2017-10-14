#include <windows.h>
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/legacy/legacy.hpp"
#include "toolsCV.h"

//�Զ��ж�ʶ��OpenCV�İ汾�ţ����ݴ���Ӷ�Ӧ�������⣨.lib�ļ����ķ���
#define CV_VERSION_ID       CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#ifdef _DEBUG
#define cvLIB(name) "opencv_" name CV_VERSION_ID "d"
#else
#define cvLIB(name) "opencv_" name CV_VERSION_ID
#endif
#pragma comment( lib, cvLIB("core") )
#pragma comment( lib, cvLIB("imgproc") )
#pragma comment( lib, cvLIB("highgui") )
#pragma comment( lib, cvLIB("flann") )
#pragma comment( lib, cvLIB("features2d") )
#pragma comment( lib, cvLIB("calib3d") )
#pragma comment( lib, cvLIB("nonfree") )

using namespace cv;
using namespace std;


//ͼ�����ƶȼ���
//1��ֱ��ͼ����
//����������������ͼ��patch(��ȻҲ��������ͼ��)���ֱ��������ͼ���ֱ��ͼ������ֱ��ͼ���й�һ����Ȼ����ĳ�־�������ı�׼�������ƶȵĲ�����
//������˼�룺���ڼ򵥵��������ƶ�����ͼ�����ƶȽ��ж�����
//�ŵ㣺ֱ��ͼ�ܹ��ܺõĹ�һ��������256��bin������ô��ʹ�ǲ�ͬ�ֱ��ʵ�ͼ�񶼿���ֱ��ͨ����ֱ��ͼ���������ƶȣ����������С��Ƚ��ʺ����������Զ��ָ��ͼ��
//ȱ�㣺ֱ��ͼ��Ӧ����ͼ��Ҷ�ֵ�ø��ʷֲ�����ͼ��ռ�λ����Ϣ����ˣ��������У���Ϣ��ʧ���ϴ���˵�һ��ͨ��ֱ��ͼ����ƥ���Ե��е��������ġ�
//������������ǻҶ�ͼ�����趨�ĻҶȼ�Ϊ8����0-255��

double getHistSimilarity(const Mat& I1, const Mat& I2)
{
	int histSize = 256;
	float range[] = {0,256};
	const float* histRange = {range};
	bool uniform = true;
	bool accumulate = false;

	Mat hist1,hist2;

	calcHist(&I1,1,0,Mat(),hist1,1,&histSize,&histRange,uniform,accumulate);
	normalize(hist1,hist1,0,1,NORM_MINMAX,-1,Mat());

	calcHist(&I2,1,0,Mat(),hist2,1,&histSize,&histRange,uniform,accumulate);
	normalize(hist2,hist2,0,1,NORM_MINMAX,-1,Mat());

	return compareHist(hist1, hist2, CV_COMP_CORREL);

}


//2�����������㷽��
//����������ͳ������ͼ��patch��ƥ�������������������Ƶ��������������������Ϊ�����ƣ���ƥ��
//����˼�룺ͼ�������������������������sift�����㣬LK�������еĽǵ�ȵȡ��������ƶȵĲ�����ת��Ϊ�������ƥ���ˡ�
//��ǰ����һЩʵ�飬����������ƥ��ģ���һ��ͼ����з���任��Ȼ��ƥ������֮��������㣬ѡȡ����������sift�Ϳ��ٵ�sift���ΰ汾surf�ȡ�
//�����ŵ㣺�ܱ�ѡ��������Ĵ���Ҫ���㲻���ԣ��߶Ȳ����ԣ���ת����ȡ�����ͼ������ƶȼ���Ҳ�;߱�����Щ�����ԡ�
//����ȱ�㣺�������ƥ������ٶȱȽ�����ͬʱ������Ҳ�п��ܳ��ִ���ƥ�������


//3�����ڷ�ֵ����ȣ�PSNR���ķ���
//����������ѹ����Ƶ������ϸ΢�����ʱ�򣬾���Ҫ����һ���ܹ���֡�Ƚϲ���Ƶ�����ϵͳ����
//���õıȽ��㷨��PSNR( Peak signal-to-noise ratio)�����Ǹ�ʹ�á��ֲ���ֵ�����жϲ������򵥵ķ�����������������ͼ��I1��I2�����ǵ��������ֱ���i��j����c��ͨ����ÿ�����ص�ÿ��ͨ����ֵռ��һ���ֽڣ�ֵ��[0,255]��ע�⵱����ͼ�����ͬ�Ļ���MSE��ֵ����0�������ᵼ��PSNR�Ĺ�ʽ�����0�����û�����塣����������Ҫ�����Ĵ�����������������������������صĶ�̬��Χ�ܹ㣬�ڴ���ʱ��ʹ�ö����任����С��Χ��
//�ڿ���ѹ�������Ƶʱ�����ֵ��Լ��30��50֮�䣬����Խ�������ѹ������Խ�á����ͼ���������ԣ��Ϳ��ܻ�õ�15�������͵�ֵ��PSNR�㷨�򵥣������ٶ�Ҳ�ܿ졣��������ֵĲ���ֵ��ʱ����˵����۸��ܲ��ɱ���������������һ�ֳ��� �ṹ������ ���㷨�������ⷽ��ĸĽ���
double getPSNR(const Mat& I1, const Mat& I2)  
{
	Mat s1;
	absdiff(I1, I2, s1);       // |I1 - I2|
	s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
	s1 = s1.mul(s1);           // |I1 - I2|^2

	Scalar s = sum(s1);         // sum elements per channel

	double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

	if( sse <= 1e-10) // for small values return zero
		return 0;
	else
	{
		double  mse =sse /(double)(I1.channels() * I1.total());
		double psnr = 10.0*log10((255*255)/mse);
		return psnr;
	}
}

//���������������Ĵ��룬δ����
double psnr(Mat &I1, Mat &I2)
{
	Mat s1;
	absdiff(I1, I2, s1);
	s1.convertTo(s1, CV_32F);//ת��Ϊ32λ��float���ͣ�8λ���ܼ���ƽ��
	s1 = s1.mul(s1);
	Scalar s = sum(s1);  //����ÿ��ͨ���ĺ�
	double sse = s.val[0] + s.val[1] + s.val[2];
	if( sse <= 1e-10) // for small values return zero
		return 0;
	else
	{
		double mse = sse / (double)(I1.channels() * I1.total()); //  sse/(w*h*3)
		double psnr = 10.0 * log10((255*255)/mse);
		return psnr;
	}
}

//4�����ڽṹ�����ԣ�SSIM,structural similarity (SSIM) index measurement���ķ���
//�ṹ������������Ϊ����Ȼͼ���ź��Ǹ߶Ƚṹ���ģ������ؼ��к�ǿ������ԣ��ر��ǿ�������ӽ������أ�����������̺����Ӿ�����������ṹ����Ҫ��Ϣ��HVS����Ҫ�����Ǵ���Ұ����ȡ�ṹ��Ϣ�������öԽṹ��Ϣ�Ķ�����Ϊͼ���֪�����Ľ��ơ��ṹ������������һ�ֲ�ͬ������ģ��HVS�ͽ׵���ɽṹ��ȫ��˼�룬�����HVS���Եķ�����ȣ������������Զ��������Ե����ϵ�������һ��˼��Ĺؼ��ǴӶԸ�֪���������Ը�֪�ṹʧ�������ת�䡣��û����ͼͨ���ۼ�����������ѧ����֪ģʽ�йص����������ͼ������������ֱ�ӹ����������ӽṹ�źŵĽṹ�ı䣬�Ӷ���ĳ�̶ֳ����ƿ�����Ȼͼ�����ݸ����Լ���ͨ��ȥ��ص�����.��Ϊ�ṹ���������۵�ʵ�֣��ṹ���ƶ�ָ����ͼ����ɵĽǶȽ��ṹ��Ϣ����Ϊ���������ȡ��Աȶȵģ���ӳ����������ṹ�����ԣ�����ʧ�潨ģΪ���ȡ��ԱȶȺͽṹ������ͬ���ص���ϡ��þ�ֵ��Ϊ���ȵĹ��ƣ���׼����Ϊ�ԱȶȵĹ��ƣ�Э������Ϊ�ṹ���Ƴ̶ȵĶ�����
//������scalar��ʽ���棬��ȡscalar�ڵ����ݼ��ɻ�ȡ��Ӧ�����ƶ�ֵ������ֵ�ķ�Χ��0��1֮�䣬1Ϊ��ȫһ�£�0Ϊ��ȫ��һ����
Scalar getMSSIM( const Mat& i1, const Mat& i2)
{
	assert(i1.rows==i2.rows && i1.cols==i2.cols);			//ͼ���Сһ�����ܱȽ�������

	const double C1 = 6.5025, C2 = 58.5225;
	/***************************** INITS **********************************/
	int d     = CV_32F;

	Mat I1, I2;
	i1.convertTo(I1, d);           // cannot calculate on one byte large values
	i2.convertTo(I2, d);

	Mat I2_2   = I2.mul(I2);        // I2^2
	Mat I1_2   = I1.mul(I1);        // I1^2
	Mat I1_I2  = I1.mul(I2);        // I1 * I2

	/*************************** END INITS **********************************/

	Mat mu1, mu2;   // PRELIMINARY COMPUTING
	GaussianBlur(I1, mu1, Size(11, 11), 1.5);
	GaussianBlur(I2, mu2, Size(11, 11), 1.5);

	Mat mu1_2   =   mu1.mul(mu1);
	Mat mu2_2   =   mu2.mul(mu2);
	Mat mu1_mu2 =   mu1.mul(mu2);

	Mat sigma1_2, sigma2_2, sigma12;

	GaussianBlur(I1_2, sigma1_2, Size(11, 11), 1.5);
	sigma1_2 -= mu1_2;

	GaussianBlur(I2_2, sigma2_2, Size(11, 11), 1.5);
	sigma2_2 -= mu2_2;

	GaussianBlur(I1_I2, sigma12, Size(11, 11), 1.5);
	sigma12 -= mu1_mu2;

	///////////////////////////////// FORMULA ////////////////////////////////
	Mat t1, t2, t3;

	t1 = 2 * mu1_mu2 + C1;
	t2 = 2 * sigma12 + C2;
	t3 = t1.mul(t2);              // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

	t1 = mu1_2 + mu2_2 + C1;
	t2 = sigma1_2 + sigma2_2 + C2;
	t1 = t1.mul(t2);               // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

	Mat ssim_map;
	divide(t3, t1, ssim_map);      // ssim_map =  t3./t1;

	Scalar mssim = mean( ssim_map ); // mssim = average of ssim map
	return mssim;
}

//��һ����ʽ�����������һ��
double ssim(Mat &i1, Mat & i2)
{
	const double C1 = 6.5025, C2 = 58.5225;
	int d = CV_32F;
	Mat I1, I2;
	i1.convertTo(I1, d);
	i2.convertTo(I2, d);
	Mat I1_2 = I1.mul(I1);
	Mat I2_2 = I2.mul(I2);
	Mat I1_I2 = I1.mul(I2);
	Mat mu1, mu2;
	GaussianBlur(I1, mu1, Size(11,11), 1.5);
	GaussianBlur(I2, mu2, Size(11,11), 1.5);
	Mat mu1_2 = mu1.mul(mu1);
	Mat mu2_2 = mu2.mul(mu2);
	Mat mu1_mu2 = mu1.mul(mu2);
	Mat sigma1_2, sigam2_2, sigam12;
	GaussianBlur(I1_2, sigma1_2, Size(11, 11), 1.5);
	sigma1_2 -= mu1_2;

	GaussianBlur(I2_2, sigam2_2, Size(11, 11), 1.5);
	sigam2_2 -= mu2_2;

	GaussianBlur(I1_I2, sigam12, Size(11, 11), 1.5);
	sigam12 -= mu1_mu2;
	Mat t1, t2, t3;
	t1 = 2 * mu1_mu2 + C1;
	t2 = 2 * sigam12 + C2;
	t3 = t1.mul(t2);

	t1 = mu1_2 + mu2_2 + C1;
	t2 = sigma1_2 + sigam2_2 + C2;
	t1 = t1.mul(t2);

	Mat ssim_map;
	divide(t3, t1, ssim_map);
	Scalar mssim = mean(ssim_map);
//	return mssim;
	double ssim = (mssim.val[0] + mssim.val[1] + mssim.val[2]) /3;
	return ssim;
}

//ͼ��ģ��ƥ��
//
//һ����ԣ�Դͼ����ģ��ͼ��patch�ߴ�һ���Ļ�������ֱ��ʹ��������ܵ�ͼ�����ƶȲ����ķ��������Դͼ����ģ��ͼ��ߴ粻һ����ͨ����Ҫ���л���ƥ�䴰�ڣ�ɨ�������ͼ������õ�ƥ��patch��
//
//��OpenCV�ж�Ӧ�ĺ���Ϊ��matchTemplate()������������������ͼ���л�������Ѱ�Ҹ���λ����ģ��ͼ��patch�����ƶȡ����ƶȵ����۱�׼��ƥ�䷽�����У�CV_TM_SQDIFFƽ����ƥ�䷨�����ƶ�Խ�ߣ�ֵԽС����CV_TM_CCORR���ƥ�䷨�����ó˷����������ƶ�Խ��ֵԽ�󣩣�CV_TM_CCOEFF���ϵ��ƥ�䷨��1��ʾ��õ�ƥ�䣬-1��ʾ����ƥ�䣩��
//
//��һ���µ������������ƶȻ��߽��о�������ķ�����EMD��Earth Mover��s Distances
//
//EMD is defined as the minimal cost that must be paid to transform one histograminto the other, where there is a ��ground distance�� between the basic featuresthat are aggregated into the histogram��
//
//���߱仯������ͼ����ɫֵ��Ư�ƣ�����Ư��û�иı���ɫֱ��ͼ����״����Ư����������ɫֵλ�õı仯���Ӷ����ܵ���ƥ�����ʧЧ����EMD��һ�ֶ���׼�򣬶���������һ��ֱ��ͼת��Ϊ��һ��ֱ��ͼ����״�������ƶ�ֱ��ͼ�Ĳ��֣���ȫ������һ���µ�λ�ã�����������ά�ȵ�ֱ��ͼ�Ͻ������ֶ�����
//
//��OpenCV������Ӧ�ļ��㷽����cvCalcEMD2()�������opencv֧�ֿ⣬����ֱ��ͼ�������ԭͼ��HSV��ɫ�ռ�ֱ��ͼ֮���EMD���롣


//��װ���ٲ��ԣ�������װ������һ����������ԣ�
double match(cv::Mat image, cv::Mat tepl, cv::Point &point, int method)
{
    int result_cols =  image.cols - tepl.cols + 1;
    int result_rows = image.rows - tepl.rows + 1;

    cv::Mat result = cv::Mat( result_cols, result_rows, CV_32FC1 );
    cv::matchTemplate( image, tepl, result, method );

    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, Mat() );

    switch(method)
    {
    case CV_TM_SQDIFF:
    case CV_TM_SQDIFF_NORMED:
        point = minLoc;
        return minVal;
        break;

    default:
        point = maxLoc;
        return maxVal;
        break;
    }
}

//���ǻ�����ȥ��ģ���С��ƥ��ȵ�Ӱ�죺
double match1(cv::Mat image, cv::Mat tepl, cv::Point &point, int method)
{
    int result_cols =  image.cols - tepl.cols + 1;
    int result_rows = image.rows - tepl.rows + 1;

    cv::Mat result = cv::Mat( result_cols, result_rows, CV_32FC1 );
    cv::matchTemplate( image, tepl, result, method );

    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, Mat() );

    switch(method)
    {
    case CV_TM_SQDIFF:
        point = minLoc;
        return minVal / (tepl.cols * tepl.cols);
        break;
    case CV_TM_SQDIFF_NORMED:
        point = minLoc;
        return minVal;
        break;
    case CV_TM_CCORR:
    case CV_TM_CCOEFF:
        point = maxLoc;
        return maxVal / (tepl.cols * tepl.cols);
        break;
    case CV_TM_CCORR_NORMED:
    case CV_TM_CCOEFF_NORMED:
    default:
        point = maxLoc;
        return maxVal;
        break;
    }
}

int mat2solution(Mat boardPath, Point p)
{
	int width = 40;		//�ֱ���	Point p(52, 62);
	
	int dx=54;

	double rMax=0;

	Mat rRook;
	Mat lRook = boardPath(Rect(p.x-width/2, p.y-width/2, width, width));
	imwrite("lRook.jpg", lRook);

	Point ptRook;
	//����dx, �ٶ�dy=dx
	for(int RRookx = p.x+8*width; RRookx < boardPath.cols-width/2 ; RRookx++)		//�����ٵ� 8���ӵ�x��ʼ���ɽ�ʡ��������
	{
		rRook = boardPath(Rect(RRookx - width/2, p.y-width/2, width, width));

		double r= ssim(lRook,rRook);

		//�����ƶ�������һ�����϶�Ϊ�ҳ��ˣ�û������ֵ����˿��ܲ�׼ȷ
		if(r>rMax) 
		{
			rMax=r;
			ptRook.x=RRookx;
			ptRook.y=p.y;
			dx=(RRookx-p.x)/8;
			//dy=dx;

			cout<<  "ssim:  " << r << endl;
			cout<<  "dx:  " << dx << endl;
			imwrite("rRook.jpg",rRook);
			//imshow("��ѡ�ҳ���", rRook);
			//waitKey(0);
		}
	}
	
	return dx;
}

int myCanny()
{
	//����ԭʼͼ    
    Mat src = imread("qq.jpg");  //����Ŀ¼��Ӧ����һ����Ϊ1.jpg���ز�ͼ  
    Mat src1=src.clone();  
  
    //��ʾԭʼͼ   
    //imshow("��ԭʼͼ��Canny��Ե���", src);   
  
    //----------------------------------------------------------------------------------  
    //  һ����򵥵�canny�÷����õ�ԭͼ��ֱ���á�  
    //----------------------------------------------------------------------------------  
    Canny( src, src, 150, 100,3 );  
    imshow("��Ч��ͼ��Canny��Ե���", src);   
  
      
    //----------------------------------------------------------------------------------  
    //  �����߽׵�canny�÷���ת�ɻҶ�ͼ�����룬��canny����󽫵õ��ı�Ե��Ϊ���룬����ԭͼ��Ч��ͼ�ϣ��õ���ɫ�ı�Եͼ  
    //----------------------------------------------------------------------------------  
    Mat dst,edge,gray;  
  
    // ��1��������srcͬ���ͺʹ�С�ľ���(dst)  
    dst.create( src1.size(), src1.type() );  
  
    // ��2����ԭͼ��ת��Ϊ�Ҷ�ͼ��  
    cvtColor( src1, gray, CV_BGR2GRAY );  
  
    // ��3������ʹ�� 3x3�ں�������  
    blur( gray, edge, Size(3,3) );  
  
    // ��4������Canny����  
    Canny( edge, edge, 3, 9,3 );  
    //imshow("��Ч��ͼ������Canny���� ", edge);   
  
    //��5����g_dstImage�ڵ�����Ԫ������Ϊ0   
    dst = Scalar::all(0);  
    //imshow("��Ч��ͼ����g_dstImage�ڵ�����Ԫ������Ϊ0", dst);   
  
    //��6��ʹ��Canny��������ı�Եͼg_cannyDetectedEdges��Ϊ���룬����ԭͼg_srcImage����Ŀ��ͼg_dstImage��  
    src1.copyTo( dst, edge);  
  
    //��7����ʾЧ��ͼ   
    //imshow("��Ч��ͼ��Canny��Ե���2", dst);   
  
    waitKey(0);   
  
    return 0;  
}

int mySobel( )  
{  
    //��0������ grad_x �� grad_y ����  
    Mat grad_x, grad_y;  
    Mat abs_grad_x, abs_grad_y,dst;  
  
    //��1������ԭʼͼ    
    Mat src = imread("board.png");  //����Ŀ¼��Ӧ����һ����Ϊ1.jpg���ز�ͼ  
  
    //��2����ʾԭʼͼ   
    imshow("��ԭʼͼ��sobel��Ե���", src);   
  
    //��3���� X�����ݶ�  
    Sobel( src, grad_x, CV_16S, 1, 0, 3, 1, 1, BORDER_DEFAULT );  
    convertScaleAbs( grad_x, abs_grad_x );  
    imshow("��Ч��ͼ�� X����Sobel", abs_grad_x);   
  
    //��4����Y�����ݶ�  
    Sobel( src, grad_y, CV_16S, 0, 1, 3, 1, 1, BORDER_DEFAULT );  
    convertScaleAbs( grad_y, abs_grad_y );  
    imshow("��Ч��ͼ��Y����Sobel", abs_grad_y);   
  
    //��5���ϲ��ݶ�(����)  
    addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, dst );  
    imshow("��Ч��ͼ�����巽��Sobel", dst);   
  
    waitKey(0);   
    return 0;   
}  



//-----------------------------------������˵����----------------------------------------------??
//??????��������:������OpenCV���Ž̳�֮ʮ�ߡ�OpenCV��ӳ��?&?SURF��������ϼ�?��?��������Դ��?֮��SURF�������⡿??
//??????��������IDE�汾��Visual?Studio?2010??
//??????��������OpenCV�汾��???2.4.9??
//??????2014��6��15��?Created?by?ǳī??
//??????���ײ������ӣ�?http://blog.csdn.net/poem_qianmo/article/details/30974513??
//??????PS:��������ϲ���ѧϰЧ������??
//??????ǳī��΢����@ǳī_ë����?http://weibo.com/1723155442??
//??????ǳī��֪����http://www.zhihu.com/people/mao-xing-yun??
//??????ǳī�Ķ��꣺http://www.douban.com/people/53426472/??
//----------------------------------------------------------------------------------------------??

int myKeyPoint()
{
	//��1������ԴͼƬ����ʾ??
	Mat srcImage1= imread("board.png",1);
	Mat srcImage2 = imread("pai.jpg",1);
	if(!srcImage1.data|| !srcImage2.data)//����Ƿ��ȡ�ɹ�??
	{ 
		printf("��ȡͼƬ������ȷ��Ŀ¼���Ƿ���imread����ָ�����Ƶ�ͼƬ����~��\n");
		waitKey(0);
		return false;
	}
	imshow("ԭʼͼ1",srcImage1);
	imshow("ԭʼͼ2",srcImage2);
	
	//��2��������Ҫ�õ��ı�������??
	int minHessian = 400;//����SURF�е�hessian��ֵ������������??
	SurfFeatureDetector detector(minHessian);//����һ��SurfFeatureDetector��SURF��?������������??
	std::vector<KeyPoint> keypoints_1, keypoints_2;//vectorģ�������ܹ�����������͵Ķ�̬���飬�ܹ����Ӻ�ѹ������??
	
	//��3������detect��������SURF�����ؼ��㣬������vector������??
	detector.detect( srcImage1, keypoints_1 );
	detector.detect( srcImage2, keypoints_2 );
	
	//��4�����������ؼ���??
	Mat img_keypoints_1; Mat img_keypoints_2;
	drawKeypoints(srcImage1, keypoints_1, img_keypoints_1, Scalar::all(-1), DrawMatchesFlags::DEFAULT );
	drawKeypoints(srcImage2, keypoints_2, img_keypoints_2, Scalar::all(-1), DrawMatchesFlags::DEFAULT );
	
	//��5����ʾЧ��ͼ??
	imshow("��������Ч��ͼ1", img_keypoints_1 );
	imshow("��������Ч��ͼ2", img_keypoints_2 );
	
	waitKey(0);
	
	return 0;
}


int mySurf()
{	//���Ч�����а�
	//��1������ԴͼƬ����ʾ??
	Mat srcImage1= imread("board.png",1);
	Mat srcImage2 = imread("pawn.png",1);
	if(!srcImage1.data|| !srcImage2.data)//����Ƿ��ȡ�ɹ�??
	{ 
		printf("��ȡͼƬ������ȷ��Ŀ¼���Ƿ���imread����ָ�����Ƶ�ͼƬ����~��\n");
		waitKey(0);
		return false;
	}
	imshow("ԭʼͼ1",srcImage1);
	imshow("ԭʼͼ2",srcImage2);

	//��2��ʹ��SURF���Ӽ��ؼ���??
	int minHessian = 700;	//SURF�㷨�е�hessian��ֵ??
	SurfFeatureDetector detector( minHessian );		//����һ��SurfFeatureDetector��SURF��?������������????
	std::vector<KeyPoint> keyPoint1, keyPoints2;	//vectorģ���࣬����������͵Ķ�̬����

	//��3������detect��������SURF�����ؼ��㣬������vector������??
	detector.detect( srcImage1, keyPoint1);
	detector.detect( srcImage2, keyPoints2 );

	//��4������������������������  
	SurfDescriptorExtractor extractor;  
	Mat descriptors1, descriptors2;  
	extractor.compute( srcImage1, keyPoint1, descriptors1 );  
	extractor.compute( srcImage2, keyPoints2, descriptors2 );  

	//��5��ʹ��BruteForce����ƥ��  
	// ʵ����һ��ƥ����  
	BruteForceMatcher< L2<float> > matcher;  
	std::vector< DMatch > matches;  
	//ƥ������ͼ�е������ӣ�descriptors��  
	matcher.match( descriptors1, descriptors2, matches );  

	//��6�����ƴ�����ͼ����ƥ����Ĺؼ���  
	Mat imgMatches;  
	drawMatches( srcImage1, keyPoint1, srcImage2, keyPoints2, matches, imgMatches );//���л���  

	//��7����ʾЧ��ͼ  
	imshow("ƥ��ͼ", imgMatches );  

	waitKey(0);

	return 0;

}



//----------------------------------��ROI_AddImage(?)������----------------------------------??
//?��������ROI_AddImage����??
//?????���������ø���Ȥ����ROIʵ��ͼ�����??
//----------------------------------------------------------------------------------------------??
bool ROI_AddImage()
{
	//��1������ͼ��??
	Mat srcImage1= imread("board.png");
	Mat logoImage= imread("pawn.png");
	if(!srcImage1.data)
	{
		printf("���ã���ȡsrcImage1����~��\n");
		return false;
	}
	if(!logoImage.data)
	{
		printf("���ã���ȡlogoImage����~��\n");
		return false;
	}

	//��2������һ��Mat���Ͳ������趨ROI����??
	Mat imageROI= srcImage1(Rect(320,350,120,122));
	imshow("ROI",imageROI);

	////��3��������ģ�������ǻҶ�ͼ��??
	Mat mask= imread("pai.jpg",0);

	////��4������Ĥ������ROI
	logoImage.copyTo( imageROI, mask);
	//logoImage.copyTo( imageROI, imageROI);

	//��5����ʾ���??
	//namedWindow("<1>����ROIʵ��ͼ�����ʾ������");
	//imshow("<1>����ROIʵ��ͼ�����ʾ������",srcImage1);

	waitKey(0);
	return 0;
}

//
void hwnd2mat()
{
    HWND hwnd;
    hwnd=GetDesktopWindow();
    HDC hwindowDC,hwindowCompatibleDC;

    int height,width,srcheight,srcwidth;
    HBITMAP hbwindow;
    Mat src;
    BITMAPINFOHEADER  bi;

    hwindowDC=GetDC(hwnd);
    hwindowCompatibleDC=CreateCompatibleDC(hwindowDC);
    SetStretchBltMode(hwindowCompatibleDC,COLORONCOLOR);  

    RECT windowsize;    // get the height and width of the screen
    GetClientRect(hwnd, &windowsize);

    srcheight = windowsize.bottom;
    srcwidth = windowsize.right;
    height = windowsize.bottom;  //change this to whatever size you want to resize to
    width = windowsize.right;

    src.create(height,width,CV_8UC4);

    // create a bitmap
    hbwindow = CreateCompatibleBitmap( hwindowDC, width, height);
    bi.biSize = sizeof(BITMAPINFOHEADER);   
    bi.biWidth = width;    
    bi.biHeight = -height;  //this is the line that makes it draw upside down or not
    bi.biPlanes = 1;    
    bi.biBitCount = 32;    
    bi.biCompression = BI_RGB;    
    bi.biSizeImage = 0;  
    bi.biXPelsPerMeter = 0;    
    bi.biYPelsPerMeter = 0;    
    bi.biClrUsed = 0;    
    bi.biClrImportant = 0;

    // use the previously created device context with the bitmap
    SelectObject(hwindowCompatibleDC, hbwindow);
    // copy from the window device context to the bitmap device context
    StretchBlt( hwindowCompatibleDC, 0,0, width, height, hwindowDC, 0, 0,srcwidth,srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
    GetDIBits(hwindowCompatibleDC,hbwindow,0,height,src.data,(BITMAPINFO *)&bi,DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

    // avoid memory leak,�ǳ���Ҫ���ر���ʵʱ��ȡ��Ҫѭ��ʱ������ӻ�ʹ�ڴ�������full����ʹ������
    DeleteObject (hbwindow); DeleteDC(hwindowCompatibleDC); ReleaseDC(hwnd, hwindowDC);
	
    //screenImg=src(Rect(0,0,sWidth,sHeight));//ͨ��GetDIBits����������Bitmap copy��Mat 
    //��Ϊ��������jin10.com�����ģ�right time right place,��EIA��ÿ��������11��30�����ڣ�ͬһ����ҳ��ͬһ���ط���50*25����
    //���ԣ�ֻ��Ҫ��ǰ����������ĵط���ã������ԼӸ��жϣ�ֻ��Ҫ11��30��������ݣ�
}

bool MyShowImage( const cv::Mat& img, HDC hdc, const RECT& rect )
{
	CvMat _img = img;
	const CvArr* arr = &_img;

	CvMat stub;
	CvMat* image = cvGetMat( arr, &stub );

	// ����BITMAPINFOͷ
	SIZE size = { image->width, image->height };
	int channels = 3;
	BITMAPINFO binfo;
	memset( &binfo, 0, sizeof(binfo));
	BITMAPINFOHEADER& bmih = binfo.bmiHeader;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = size.cx;
	bmih.biHeight = abs(size.cy);
	bmih.biPlanes = 1;
	bmih.biBitCount = (unsigned short)(channels*8);
	bmih.biCompression = BI_RGB;

	void* dst_ptr = 0;
	HBITMAP hb = CreateDIBSection( hdc, &binfo, DIB_RGB_COLORS, &dst_ptr, 0, 0 );

	HDC windowdc = ::CreateCompatibleDC( hdc );
	SelectObject( windowdc, hb );

	CvMat dst;
	cvInitMatHeader( &dst, size.cy, size.cx, CV_8UC3, dst_ptr, (size.cx*channels + 3)&-4 );

	int origin = ((IplImage*)arr)->origin;
	cvConvertImage(image, &dst, origin==0?CV_CVTIMG_FLIP : 0 );

	// ��ʾ
	SetStretchBltMode( hdc, COLORONCOLOR );
	//BitBlt( hdc, 0, 0, size.cx, size.cy, windowdc, 0, 0, SRCCOPY );
	StretchBlt( hdc, 0, 0, rect.right-rect.left, rect.bottom-rect.top, windowdc, 0, 0, size.cx, size.cy, SRCCOPY );

	return 0;
}

int test_Myshowimage()
{
	const char* imagename = "1.tif";

	cv::Mat img = cv::imread( imagename );
	if( img.empty() || !img.data )
	{
		fprintf(stderr, "Can not load image %s\n", imagename);
		return -1;
	}

	//cv::namedWindow("image", CV_WINDOW_AUTOSIZE);
	//cv::imshow("image", img);
	//cv::waitKey();

	// ���Ū�����壬Ȼ����ʾ��ȥ
	HWND hwnd = ::GetConsoleWindow();
	RECT rect;
	GetWindowRect( hwnd, &rect );
	MyShowImage( img, ::GetWindowDC(hwnd), rect );

	return 0;
}


// hbitmap convert to IplImage   
IplImage* hBitmapToIpl(HBITMAP hBmp)   
{   
    BITMAP bmp;    
    GetObject(hBmp,sizeof(BITMAP),&bmp);    
  
    // get channels which equal 1 2 3 or 4    
    // bmBitsPixel :   
    // Specifies the number of bits    
    // required to indicate the color of a pixel.    
    int nChannels = bmp.bmBitsPixel == 1 ? 1 : bmp.bmBitsPixel/8 ;   
  
    // get depth color bitmap or grayscale   
    int depth = bmp.bmBitsPixel == 1 ? IPL_DEPTH_1U : IPL_DEPTH_8U;    
       
       
    // create header image   
    IplImage* img = cvCreateImage(cvSize(bmp.bmWidth,bmp.bmHeight),depth,nChannels);    
       
    // allocat memory for the pBuffer   
    BYTE *pBuffer = new BYTE[bmp.bmHeight*bmp.bmWidth*nChannels];    
       
    // copies the bitmap bits of a specified device-dependent bitmap into a buffer   
    GetBitmapBits(hBmp,bmp.bmHeight*bmp.bmWidth*nChannels,pBuffer);    
       
    // copy data to the imagedata   
    memcpy(img->imageData,pBuffer,bmp.bmHeight*bmp.bmWidth*nChannels);   
    delete pBuffer;    
  
    // create the image   
    IplImage *dst = cvCreateImage(cvGetSize(img),img->depth,3);   
    // convert color   
    cvCvtColor(img,dst,CV_BGRA2BGR);   
    cvReleaseImage(&img);   
    return dst;   
} 

//���кϲ����£�
cv::Mat mergeRows(cv::Mat A, cv::Mat B)
{
	// cv::CV_ASSERT(A.cols == B.cols&&A.type() == B.type());
	int totalRows = A.rows + B.rows;
	cv::Mat mergedDescriptors(totalRows, A.cols, A.type());
	cv::Mat submat = mergedDescriptors.rowRange(0, A.rows);
	A.copyTo(submat);
	submat = mergedDescriptors.rowRange(A.rows, totalRows);
	B.copyTo(submat);
	return mergedDescriptors;
}

//���кϲ����£�
cv::Mat mergeCols(cv::Mat A, cv::Mat B)
{
	// cv::CV_ASSERT(A.cols == B.cols&&A.type() == B.type());
	int totalCols = A.cols + B.cols;
	cv::Mat mergedDescriptors(A.rows,totalCols, A.type());
	cv::Mat submat = mergedDescriptors.colRange(0, A.cols);
	A.copyTo(submat);
	submat = mergedDescriptors.colRange(A.cols, totalCols);
	B.copyTo(submat);
	return mergedDescriptors;
}


//�������������Ľ����������ɶ��ղο�screenCapture.cpp
Mat hwnd2mat(HWND hwnd)
{

    HDC hwindowDC,hwindowCompatibleDC;

    int height,width,srcheight,srcwidth;
    HBITMAP hbwindow;
    Mat src;
    BITMAPINFOHEADER  bi;

    hwindowDC=GetDC(hwnd);
    hwindowCompatibleDC=CreateCompatibleDC(hwindowDC);
    SetStretchBltMode(hwindowCompatibleDC,COLORONCOLOR);  

    RECT windowsize;    // get the height and width of the screen
    GetClientRect(hwnd, &windowsize);

    srcheight = windowsize.bottom;
    srcwidth = windowsize.right;
    height = windowsize.bottom;  //change this to whatever size you want to resize to
    width = windowsize.right;

    src.create(height,width,CV_8UC4);
    //src.create(height,width,CV_16UC4);
	

    // create a bitmap
    hbwindow = CreateCompatibleBitmap( hwindowDC, width, height);
    bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
    bi.biWidth = width;    
    bi.biHeight = -height;  //this is the line that makes it draw upside down or not
    bi.biPlanes = 1;    
    bi.biBitCount = 32;    
    bi.biCompression = BI_RGB;    
    bi.biSizeImage = 0;  
    bi.biXPelsPerMeter = 0;    
    bi.biYPelsPerMeter = 0;    
    bi.biClrUsed = 0;    
    bi.biClrImportant = 0;

    // use the previously created device context with the bitmap
    SelectObject(hwindowCompatibleDC, hbwindow);
    // copy from the window device context to the bitmap device context
    StretchBlt( hwindowCompatibleDC, 0,0, width, height, hwindowDC, 0, 0,srcwidth,srcheight, SRCCOPY);	//change SRCCOPY to NOTSRCCOPY for wacky colors !
    GetDIBits(hwindowCompatibleDC,hbwindow,0,height,src.data,(BITMAPINFO *)&bi,DIB_RGB_COLORS);			//copy from hwindowCompatibleDC to hbwindow

    // avoid memory leak
    DeleteObject (hbwindow); DeleteDC(hwindowCompatibleDC); ReleaseDC(hwnd, hwindowDC);

    return src;
}

//Note that no error handling done here to make it simple to understand but you have to do error handling in your code!
int cap()
{
    int x_size = 800, y_size = 600; // <-- Your res for the image


    HBITMAP hBitmap; // <-- The image represented by hBitmap
    Mat matBitmap; // <-- The image represented by mat

    // Initialize DCs
    HDC hdcSys = GetDC(NULL); // Get DC of the target capture..
    HDC hdcMem = CreateCompatibleDC(hdcSys); // Create compatible DC 

    void *ptrBitmapPixels; // <-- Pointer variable that will contain the potinter for the pixels

    // Create hBitmap with Pointer to the pixels of the Bitmap
    BITMAPINFO bi; HDC hdc;
    ZeroMemory(&bi, sizeof(BITMAPINFO));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = x_size;
    bi.bmiHeader.biHeight = -y_size;  //negative so (0,0) is at top left
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    hdc = GetDC(NULL);
    hBitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &ptrBitmapPixels, NULL, 0);
    // ^^ The output: hBitmap & ptrBitmapPixels

    // Set hBitmap in the hdcMem 
    SelectObject(hdcMem, hBitmap);

    // Set matBitmap to point to the pixels of the hBitmap
    matBitmap = Mat(y_size, x_size, CV_8UC4, ptrBitmapPixels, 0);
    //              ^^ note: first it is y, then it is x. very confusing
    // * SETUP DONE *
    // Now update the pixels using BitBlt
    BitBlt(hdcMem, 0, 0, x_size, y_size, hdcSys, 0, 0, SRCCOPY);

    // Just to do some image processing on the pixels.. (Dont have to to this)
    Mat matRef = matBitmap(Range(100, 200), Range(100, 200));
    //                            y1    y2          x1   x2
    bitwise_not(matRef, matRef); // Invert the colors in this x1,x2,y1,y2

    // Display the results through Mat
    //imshow("Title", matBitmap);

    // Wait until some key is pressed
    //waitKey(0);
    return 0;
}

void change_piece_size(char *path, char* dst,int size)
{
	char PieceChar[7] = { 'k', 'r', 'n', 'c', 'a', 'b', 'p'};
	char color[2] = { 'r', 'b'};
	string filename, p, c;
	string str_path;
	str_path = path;
	Mat img1, img2;
	
	//img1= imread( "E:\\link_vc2010\\RES\\mask.png" );
	//resize(img1,img2,Size(56,56),0,0,CV_INTER_LINEAR);
	//imwrite("selected.bmp", img2);
	//return;
	
	for(int j=0; j<2; j++)
	{
		for(int i=0; i<7; i++)
		{
			c= color[j];
			p= PieceChar[i];
			filename =str_path + "\\Piece\\large\\" + c + p + ".png";
			img1= imread( filename );
			if(img1.data!=NULL)
			{
				resize(img1,img2,Size(size,size),0,0,CV_INTER_LINEAR);
				imwrite(dst+c+p+".bmp", img2);
			}
		}
	}
}
