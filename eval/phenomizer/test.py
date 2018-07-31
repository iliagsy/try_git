# coding: utf-8
'''phenomizer results using selenium'''
from selenium import webdriver
import unittest


from page import MainPage



class PhenomizerWebPageTestCase(unittest.TestCase):
    def setUp(self):
        self.client = webdriver.Firefox()
        self.client.get("http://compbio.charite.de/phenomizer/")

    def _test_1(self):
        mainPage = MainPage(self.client)
        mainPage.get_diseases(['HP:0010704', 'HP:0005767', 'HP:0010711'])
        print mainPage.result

    def test_2(self):
        mainPage = MainPage(self.client)
        mainPage.get_diseases(['HP:0010704', 'HP:0005767', 'HP:0010711'])
        mainPage.get_diseases(['HP:0010706', 'HP:0001459', 'HP:0010707'])
        print mainPage.result

    def tearDown(self):
        self.client.close()


if __name__ == '__main__':
    unittest.main()
