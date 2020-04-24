"""
    Name: Hemant Koti
    UB ID: 50338178
    UB Name: hemantko
    References: https://towardsdatascience.com/image-stitching-using-opencv-817779c86a83
"""

import argparse
import os
import sys
import glob
import numpy as np
import random as rand

import cv2

from scipy.spatial.distance import cdist

def parse_args():
    """
        Parse arguments.
    """

    parser = argparse.ArgumentParser(description="CSE 473/573 Project 2.")
    parser.add_argument(
        "--img_path", type=str, default="../ubdata",
        help="path to the image used to create a panorama")
    args = parser.parse_args()
    return args

def crop_black(directory, image):
    """
        Crop all the black pixels from the image.
    """

    max_x = 0
    max_y = 0

    for i in range(image.shape[0]):
        for j in range(image.shape[1]):
            if not np.array_equal(image[i, j], np.zeros(3)):
                if i > max_y:
                   max_y = i
                if j > max_x:
                   max_x = j

    # Remove an existing panorama image
    if os.path.exists(os.path.join(directory, 'panorama.jpg')):
        os.remove(os.path.join(directory, 'panorama.jpg'))

    return image[0 : max_y, 0 : max_x]

def count_black_pixels(img):
    """
        Count the number of black pixels in an image.
    """

    count = 0
    for i in range(img.shape[0]):
        for j in range(img.shape[1]):
            if np.array_equal(img[i, j], np.zeros(3)):
                count = count + 1

    return count

def swap_images(image1, image2):
    """
        Swap any two images.
    """

    temp = image1
    image1 = image2
    image2 = temp

    return image1, image2

def match_keypoints(kps1, kps2, desc1, desc2):
    """
        Find matching descriptors and corresponding keypoints between 2 images using squared euclidean distance measure.
    """

    # Using squared euclidian distance method to calculate the distance
    distance = cdist(desc1, desc2, 'sqeuclidean')

    coordinates_in_image_1 = np.array([kps1[point].pt for point in np.where(distance < 7300)[0]])
    coordinates_in_image_2 = np.array([kps2[point].pt for point in np.where(distance < 7300)[1]])

    return np.concatenate((coordinates_in_image_1, coordinates_in_image_2), axis=1)

def ransac(matchingPoints):
    """
         Calculate Homography matrix using RANSAC algorithm.
    """

    # Ransac parameters
    max_inlier_count = -(sys.maxsize)
    max_homography = []

    for i in range(1000):

        # Select 4 random pointschoice
        random  = rand.SystemRandom()
        randomMatchingPairs = np.concatenate(([random.choice(matchingPoints)],[random.choice(matchingPoints)],
                                              [random.choice(matchingPoints)],[random.choice(matchingPoints)]), axis=0)

        Homography_Matrix = cv2.getPerspectiveTransform(np.float32(randomMatchingPairs[:,0:2]), np.float32(randomMatchingPairs[:,2:4]))

        # Avoid matrix with index less than 3
        if  np.linalg.matrix_rank(Homography_Matrix) < 3:
            continue

        # Calculate error for each point using the current homographic matrix H
        points_in_image_1 = np.concatenate((matchingPoints[:, 0:2], np.ones((len(matchingPoints), 1))), axis=1)
        points_in_image_2 = matchingPoints[:, 2:4]

        correspondingPoints = np.zeros((len(matchingPoints), 2))

        for i in range(len(matchingPoints)):
            transformationMatrix = np.matmul(Homography_Matrix, points_in_image_1[i])
            correspondingPoints[i] = (transformationMatrix / transformationMatrix[2])[0:2]

        inliers = matchingPoints[np.where((np.linalg.norm(points_in_image_2 - correspondingPoints, axis=1) ** 2) < 0.5)[0]]

        if len(inliers) > max_inlier_count:
            max_inlier_count = len(inliers)
            max_homography = Homography_Matrix.copy()

    return max_homography

def main():
    """
         Main method.
    """

    if len(sys.argv) != 2:
        print("Invalid number of arguments.")
        return

    directory = str(sys.argv[1])
    jpgImagesDirectory = os.path.join(directory, '*.jpg')

    # Remove an existing panorama image
    if os.path.exists(os.path.join(directory, 'panorama.jpg')):
        os.remove(os.path.join(directory, 'panorama.jpg'))

    grayscaleImages = [cv2.imread(file, 0) for file in glob.glob(jpgImagesDirectory)]
    colorImages = [cv2.imread(file, 1) for file in glob.glob(jpgImagesDirectory)]

    if len(grayscaleImages) == 0 or len(grayscaleImages) == 1:
        print("Cannot create a panorama for 0 or 1 images.")
        return

    # SIFT feature detection
    sift = cv2.xfeatures2d.SIFT_create()

    # Take two images and create a panorama
    if len(grayscaleImages) == 2:

        keypoints1, descriptors1 = sift.detectAndCompute(grayscaleImages[0], None)
        keypoints2, descriptors2 = sift.detectAndCompute(grayscaleImages[1], None)

        # Forward stitching - Stitching image 1 and image 2
        Homography12 = ransac(match_keypoints(keypoints1, keypoints2, descriptors1, descriptors2))
        result = cv2.warpPerspective(colorImages[0], Homography12,
                                         (
                                           int(colorImages[0].shape[1] + colorImages[1].shape[1] * 0.8),
                                           int(colorImages[0].shape[0] + colorImages[1].shape[0] * 0.4)
                                         )
                                     )

        result[0:colorImages[1].shape[0], 0:colorImages[1].shape[1]] = colorImages[1]
        cv2.imwrite(os.path.join(directory, '12.jpg'), result)

        # Backward stitching - Stitching image 2 and image 1
        Homography21 = ransac(match_keypoints(keypoints2, keypoints1, descriptors2, descriptors1))
        result = cv2.warpPerspective(colorImages[1], Homography21,
                                         (
                                           int(colorImages[1].shape[1] + colorImages[0].shape[1] * 0.8),
                                           int(colorImages[1].shape[0] + colorImages[0].shape[0] * 0.4)
                                         )
                                     )

        result[0:colorImages[0].shape[0], 0:colorImages[0].shape[1]] = colorImages[0]
        cv2.imwrite(os.path.join(directory, '21.jpg'), result)

        # Count the black pixels and get the best panorama
        colorImage12 = cv2.imread(os.path.join(directory, '12.jpg'), 1)
        colorImage21 = cv2.imread(os.path.join(directory, '21.jpg'), 1)

        if count_black_pixels(colorImage12) < count_black_pixels(colorImage21):
           os.remove(os.path.join(directory, '21.jpg'))
           os.rename(os.path.join(directory, '12.jpg'), os.path.join(directory, 'panorama.jpg'))
        else:
           os.remove(os.path.join(directory, '12.jpg'))
           os.rename(os.path.join(directory, '21.jpg'), os.path.join(directory, 'panorama.jpg'))

    # Take three images and create a panorama
    elif len(grayscaleImages) == 3:

        keypoints1, descriptors1 = sift.detectAndCompute(grayscaleImages[0], None)
        keypoints2, descriptors2 = sift.detectAndCompute(grayscaleImages[1], None)
        keypoints3, descriptors3 = sift.detectAndCompute(grayscaleImages[2], None)

        matches12 = match_keypoints(keypoints1, keypoints2, descriptors1, descriptors2)
        matches13 = match_keypoints(keypoints1, keypoints3, descriptors1, descriptors3)
        matches23 = match_keypoints(keypoints2, keypoints3, descriptors2, descriptors3)

        total_matches_image_1 = len(matches12) + len(matches13)
        total_matches_image_2 = len(matches12) + len(matches23)
        total_matches_image_3 = len(matches13) + len(matches23)

        if total_matches_image_1 >= total_matches_image_2 and total_matches_image_1  >= total_matches_image_3:
            anchorIndex = 0
        elif total_matches_image_2 >= total_matches_image_1 and total_matches_image_2 >= total_matches_image_3:
            anchorIndex = 1
        else:
            anchorIndex = 2

        if anchorIndex == 0:
            # Swap 1st and 2nd images
            grayscaleImages[0], grayscaleImages[1] = swap_images(grayscaleImages[0], grayscaleImages[1])
            colorImages[0], colorImages[1] = swap_images(colorImages[0], colorImages[1])
        elif anchorIndex == 2:
            # Swap 2nd and 3rd images
            grayscaleImages[1], grayscaleImages[2] = swap_images(grayscaleImages[1], grayscaleImages[2])
            colorImages[1], colorImages[2] = swap_images(colorImages[1], colorImages[2])

        # Get the new keypoints and descriptors
        keypoints1, descriptors1 = sift.detectAndCompute(grayscaleImages[0], None)
        keypoints2, descriptors2 = sift.detectAndCompute(grayscaleImages[1], None)
        keypoints3, descriptors3 = sift.detectAndCompute(grayscaleImages[2], None)

        # Forward Stitching - Start stitching from image 1 and image 2
        Homography12 = ransac(match_keypoints(keypoints1, keypoints2, descriptors1, descriptors2))
        result = cv2.warpPerspective(colorImages[0], Homography12,
                                         (
                                           int(colorImages[0].shape[1] + colorImages[1].shape[1] * 0.8),
                                           int(colorImages[0].shape[0] + colorImages[1].shape[0] * 0.4)
                                         )
                                     )

        result[0:colorImages[1].shape[0], 0:colorImages[1].shape[1]] = colorImages[1]
        cv2.imwrite(os.path.join(directory, '12.jpg'), result)

        # Stitching image 12 and image 3
        grayscaleImage12 = cv2.imread(os.path.join(directory, '12.jpg'), 0)
        colorImage12 = cv2.imread(os.path.join(directory, '12.jpg'), 1)
        keypoints12, descriptors12 = sift.detectAndCompute(grayscaleImage12, None)

        Homography23 = ransac(match_keypoints(keypoints12, keypoints3, descriptors12, descriptors3))
        result = cv2.warpPerspective(colorImage12, Homography23,
                                         (
                                           int(colorImage12.shape[1] + colorImages[2].shape[1] * 0.8),
                                           int(colorImage12.shape[0] + colorImages[2].shape[0] * 0.4)
                                         )
                                     )

        result[0:colorImages[2].shape[0], 0:colorImages[2].shape[1]] = colorImages[2]

        cv2.imwrite(os.path.join(directory, '123.jpg'), result)
        os.remove(os.path.join(directory, '12.jpg'))

        # Backward Stitching - Start stitching from image 3 and image 2
        Homography32 = ransac(match_keypoints(keypoints3, keypoints2, descriptors3, descriptors2))
        result = cv2.warpPerspective(colorImages[2], Homography32,
                                         (
                                           int(colorImages[2].shape[1] + colorImages[1].shape[1] * 0.8),
                                           int(colorImages[2].shape[0] + colorImages[1].shape[0] * 0.4)
                                         )
                                     )

        result[0:colorImages[1].shape[0], 0:colorImages[1].shape[1]] = colorImages[1]

        cv2.imwrite(os.path.join(directory, '32.jpg'), result)

        # Stitching image 32 and image 1
        grayscaleImage23 = cv2.imread(os.path.join(directory, '32.jpg'), 0)
        colorImage32 = cv2.imread(os.path.join(directory, '32.jpg'), 1)
        keypoints32, descriptors32 = sift.detectAndCompute(grayscaleImage23, None)

        Homography21 = ransac(match_keypoints(keypoints32, keypoints1, descriptors32, descriptors1))
        result = cv2.warpPerspective(colorImage32, Homography21,
                                         (
                                           int(colorImage32.shape[1] + colorImages[0].shape[1] * 0.8),
                                           int(colorImage32.shape[0] + colorImages[0].shape[0] * 0.4)
                                         )
                                     )

        result[0:colorImages[0].shape[0], 0:colorImages[0].shape[1]] = colorImages[0]

        cv2.imwrite(os.path.join(directory, '321.jpg'), result)
        os.remove(os.path.join(directory, '32.jpg'))

        # Count the black pixels and get the best panorama
        colorImage123 = cv2.imread(os.path.join(directory, '123.jpg'), 1)
        colorImage321 = cv2.imread(os.path.join(directory, '321.jpg'), 1)

        if count_black_pixels(colorImage123) < count_black_pixels(colorImage321):
           os.remove(os.path.join(directory, '321.jpg'))
           os.rename(os.path.join(directory,'123.jpg'), os.path.join(directory, 'panorama.jpg'))
        else:
           os.remove(os.path.join(directory, '123.jpg'))
           os.rename(os.path.join(directory, '321.jpg'), os.path.join(directory, 'panorama.jpg'))

    # For images length greater than 4 build the panorama one by one from the folder
    elif len(grayscaleImages) >= 4:

        # Run the logic until there is only image left, i.e., the panorama image
        while(len(grayscaleImages) != 1):

            list_matches_forward = []
            list_matches_backward = []
            list_max_matches = []

            for i in range(1, len(grayscaleImages)):
                keypoints1, descriptors1 = sift.detectAndCompute(grayscaleImages[0], None)
                keypoints2, descriptors2 = sift.detectAndCompute(grayscaleImages[i], None)

                matchingPointsForward = match_keypoints(keypoints1, keypoints2, descriptors1, descriptors2)
                matchingPointsBackward = match_keypoints(keypoints2, keypoints1, descriptors2, descriptors1)

                list_max_matches.append(matchingPointsForward.shape[0])
                list_matches_forward.append(matchingPointsForward)
                list_matches_backward.append(matchingPointsBackward)

            max_index = list_max_matches.index(max(list_max_matches))
            forward = list_matches_forward[max_index]
            backward = list_matches_backward[max_index]

            # Find the homography matrices for backward and forward stitching
            Homography_Forward = ransac(forward)
            Homography_Backward = ransac(backward)

            resultForwardColor = cv2.warpPerspective(colorImages[0], Homography_Forward,
                                             (
                                               int(colorImages[0].shape[1] + colorImages[max_index + 1].shape[1] * 0.8),
                                               int(colorImages[0].shape[0] + colorImages[max_index + 1].shape[0] * 0.4)
                                             )
                                         )
            resultForwardColor[0:colorImages[max_index + 1].shape[0], 0:colorImages[max_index + 1].shape[1]] = colorImages[max_index + 1]

            resultBackwardColor = cv2.warpPerspective(colorImages[max_index + 1], Homography_Backward,
                                             (
                                               int(colorImages[max_index + 1].shape[1] + colorImages[0].shape[1] * 0.8),
                                               int(colorImages[max_index + 1].shape[0] + colorImages[0].shape[0] * 0.4)
                                             )
                                         )
            resultBackwardColor[0:colorImages[0].shape[0], 0:colorImages[0].shape[1]] = colorImages[0]

            if count_black_pixels(resultBackwardColor) <= count_black_pixels(resultForwardColor):
                result = cv2.cvtColor(resultBackwardColor, cv2.COLOR_BGR2GRAY)
                resultColor = resultBackwardColor
            else:
                result = cv2.cvtColor(resultForwardColor, cv2.COLOR_BGR2GRAY)
                resultColor = resultForwardColor

            resultColor = crop_black(directory, resultColor)
            result = crop_black(directory, result)

            grayscaleImages.pop(0)
            grayscaleImages.pop(max_index)
            grayscaleImages.append(result)

            colorImages.pop(0)
            colorImages.pop(max_index)
            colorImages.append(resultColor)

        cv2.imwrite(os.path.join(directory, 'panorama.jpg'), colorImages[0])

    # Resize the final panorama
    finalColorPanorama = crop_black(directory, cv2.imread(os.path.join(directory, 'panorama.jpg'), 1))

    # Save the cropped image
    cv2.imwrite(os.path.join(directory, 'panorama.jpg'), finalColorPanorama)
    print('Panorama creation completed')

if __name__ == "__main__":
    main()





