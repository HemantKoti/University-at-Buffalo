"""
Character Detection

The goal of this task is to experiment with template matching techniques. Specifically, the task is to find ALL of
the coordinates where a specific character appears using template matching.

There are 3 sub tasks:
1. Detect character 'a'.
2. Detect character 'b'.
3. Detect character 'c'.

You need to customize your own templates. The templates containing character 'a', 'b' and 'c' should be named as
'a.jpg', 'b.jpg', 'c.jpg' and stored in './data/' folder.

Please complete all the functions that are labelled with '# TODO'. When implementing the functions,
comment the lines 'raise NotImplementedError' instead of deleting them. The functions defined in utils.py
and the functions you implement in task1.py are of great help.

Do NOT modify the code provided.
Do NOT use any API provided by opencv (cv2) and numpy (np) in your code.
Do NOT import any library (function, module, etc.).
"""


import argparse
import json
import os
import math

import utils
import task1

def parse_args():
    parser = argparse.ArgumentParser(description="cse 473/573 project 1.")
    parser.add_argument(
        "--img_path", type=str, default="./data/proj1-task2-png.png",
        help="path to the image used for character detection (do not change this arg)")
    parser.add_argument(
        "--template_path", type=str, default="./data/a.png",
        choices=["./data/a.jpg", "./data/b.jpg", "./data/c.jpg"],
        help="path to the template image")
    parser.add_argument(
        "--result_saving_directory", dest="rs_directory", type=str, default="./results/",
        help="directory to which results are saved (do not change this arg)")
    args = parser.parse_args()
    return args

def sum_of_all_elements(image):
    sum_elements = 0
    
    for row in image:
        for column in row:
            sum_elements += column
            
    return sum_elements

def subtract_elements_with_average(image, average):
    elements = task1.np.zeros((len(image), len(image)))
    
    for i, row in enumerate(image):
        for j, column in enumerate(row):
            elements[i][j] = column - average
            
    return elements

def normalized_cross_correlation(std_dev_image, std_dev_template):
    
    std_dev_image_square = utils.elementwise_mul(std_dev_image, std_dev_image)
    std_dev_template_square = utils.elementwise_mul(std_dev_template, std_dev_template)
    
    std_dev_image_square_sum = sum_of_all_elements(std_dev_image_square)
    std_dev_template_square_sum = sum_of_all_elements(std_dev_template_square)
    
    return (sum_of_all_elements(utils.elementwise_mul(std_dev_image, std_dev_template)) / 
            ((std_dev_image_square_sum ** 0.5) * (std_dev_template_square_sum ** 0.5)))

    
def detect(img, template):
    """Detect a given character, i.e., the character in the template image.

    Args:
        img: nested list (int), image that contains character to be detected.
        template: nested list (int), template image.

    Returns:
        coordinates: list (tuple), a list whose elements are coordinates where the character appears.
            format of the tuple: (x (int), y (int)), x and y are integers.
            x: row that the character appears (starts from 0).
            y: column that the character appears (starts from 0).
    """
    # TODO: implement this function.

    template_length = len(template)
    img_length = len(img)
    template_average = sum_of_all_elements(template) / (template_length ** 2)
    image_threshold = 0.6295 # Derived from experimentation
    coordinates = []
    
    ncc_value_mat = task1.np.zeros((img_length, img_length))
    
    padding_size = math.ceil((template_length - 1) / 2)
    padded_image = utils.zero_pad(img, padding_size, padding_size)
            
    for i, row in enumerate(img):
        for j, column in enumerate(row):
            # Extract image by template size and calculate it's average
            extracted_image = utils.crop(padded_image, i, i + template_length, j, j + template_length)
            extracted_image_average = sum_of_all_elements(extracted_image) / (len(extracted_image) ** 2)
            
            # Find standard deviation of image and the template
            std_dev_image = subtract_elements_with_average(extracted_image, extracted_image_average)
            std_dev_template = subtract_elements_with_average(template, template_average)
            
            ncc_value_mat[i][j] = normalized_cross_correlation(std_dev_image, std_dev_template)            
         
    for x, row in enumerate(ncc_value_mat):
        for y, column in enumerate(row):
            if column > image_threshold:
                coordinates.append((y - padding_size, x - padding_size))

    return coordinates

def save_results(coordinates, template, template_name, rs_directory):
    results = {}
    results["coordinates"] = sorted(coordinates, key=lambda x: x[0])
    results["templat_size"] = (len(template), len(template[0]))
    with open(os.path.join(rs_directory, template_name), "w") as file:
        json.dump(results, file)


def main():
    args = parse_args()

    img = task1.read_image(args.img_path)
    template = task1.read_image(args.template_path)
    
    coordinates = detect(img, template)
    
    template_name = "{}.json".format(os.path.splitext(os.path.split(args.template_path)[1])[0])
    save_results(coordinates, template, template_name, args.rs_directory)

if __name__ == "__main__":
    main()